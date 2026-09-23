#include <iostream>

#ifdef _WIN32
int main() {
    std::cout << "[SKIP] Linux IPC socket tests are only run on POSIX/Linux platforms.\n";
    return 0;
}
#else

#include "discord_ipc.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <atomic>
#include <thread>
#include <string>
#include <vector>
#include <cstring>
#include <cassert>
#include <chrono>

// Helper to recursively remove a directory
static void RemoveDirRecursive(const std::string& path) {
    DIR* d = opendir(path.c_str());
    if (!d) return;
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        std::string full_path = path + "/" + entry->d_name;
        struct stat st;
        if (lstat(full_path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                RemoveDirRecursive(full_path);
            } else {
                unlink(full_path.c_str());
            }
        }
    }
    closedir(d);
    rmdir(path.c_str());
}

static void MkdirP(const std::string& path) {
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        current += path[i];
        if (path[i] == '/' || i == path.size() - 1) {
            mkdir(current.c_str(), 0755);
        }
    }
}

// Lightweight mock Discord IPC Unix domain socket server
class MockDiscordServer {
public:
    MockDiscordServer(const std::string& sock_path) : sock_path_(sock_path) {}
    ~MockDiscordServer() { Stop(); }

    bool Start() {
        unlink(sock_path_.c_str());

        server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd_ < 0) return false;

        // Set non-blocking on server socket so accept doesn't block indefinitely
        int flags = fcntl(server_fd_, F_GETFL, 0);
        fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, sock_path_.c_str(), sizeof(addr.sun_path) - 1);

        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            close(server_fd_);
            server_fd_ = -1;
            return false;
        }

        if (listen(server_fd_, 5) != 0) {
            close(server_fd_);
            server_fd_ = -1;
            return false;
        }

        running_ = true;
        server_thread_ = std::thread([this]() {
            while (running_) {
                int client_fd = accept(server_fd_, nullptr, nullptr);
                if (client_fd >= 0) {
                    HandleClient(client_fd);
                    close(client_fd);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        });

        return true;
    }

    void Stop() {
        running_ = false;
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
        unlink(sock_path_.c_str());
    }

    bool HandshakeReceived() const { return handshake_received_.load(); }
    const std::string& ReceivedClientId() const { return client_id_; }

private:
    void HandleClient(int client_fd) {
        // Read Discord 8-byte header
        uint8_t header[8];
        ssize_t n = recv(client_fd, header, 8, 0);
        if (n == 8) {
            uint32_t opcode = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
            uint32_t len = header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24);

            if (opcode == 0 && len > 0 && len < 65536) { // Opcode 0 = Handshake
                std::vector<char> buf(len + 1, 0);
                recv(client_fd, buf.data(), len, 0);
                std::string payload(buf.data(), len);

                auto pos = payload.find("\"client_id\":\"");
                if (pos != std::string::npos) {
                    auto end_pos = payload.find("\"", pos + 13);
                    if (end_pos != std::string::npos) {
                        client_id_ = payload.substr(pos + 13, end_pos - (pos + 13));
                    }
                }
                handshake_received_.store(true);

                // Send READY frame back to keep connection alive
                std::string ready_json = "{\"cmd\":\"DISPATCH\",\"evt\":\"READY\",\"data\":{\"v\":1}}";
                uint32_t op = 1; // Frame
                uint32_t resp_len = static_cast<uint32_t>(ready_json.size());
                uint8_t resp_hdr[8];
                resp_hdr[0] = op & 0xFF; resp_hdr[1] = (op >> 8) & 0xFF; resp_hdr[2] = (op >> 16) & 0xFF; resp_hdr[3] = (op >> 24) & 0xFF;
                resp_hdr[4] = resp_len & 0xFF; resp_hdr[5] = (resp_len >> 8) & 0xFF; resp_hdr[6] = (resp_len >> 16) & 0xFF; resp_hdr[7] = (resp_len >> 24) & 0xFF;
                send(client_fd, resp_hdr, 8, MSG_NOSIGNAL);
                send(client_fd, ready_json.data(), resp_len, MSG_NOSIGNAL);

                // Keep socket open while test is connected
                while (running_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    char test_buf[1];
                    ssize_t peek = recv(client_fd, test_buf, 1, MSG_PEEK | MSG_DONTWAIT);
                    if (peek == 0) break; // Client disconnected
                }
            }
        }
    }

    std::string sock_path_;
    int server_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    std::atomic<bool> handshake_received_{false};
    std::string client_id_;
};

static std::string g_test_root;

static bool RunConnectionTest(const std::string& test_name,
                              const std::string& relative_dir,
                              int socket_index,
                              const std::string& custom_ipc_env = "") {
    std::cout << "[TEST] " << test_name << " ... " << std::flush;

    std::string full_dir = g_test_root;
    if (!relative_dir.empty()) {
        full_dir += "/" + relative_dir;
    }
    MkdirP(full_dir);
    std::string sock_path = full_dir + "/discord-ipc-" + std::to_string(socket_index);

    MockDiscordServer server(sock_path);
    if (!server.Start()) {
        std::cerr << "FAIL (could not bind socket " << sock_path << ")\n";
        return false;
    }

    if (!custom_ipc_env.empty()) {
        setenv("DISCORD_IPC_PATH", custom_ipc_env.c_str(), 1);
    } else {
        unsetenv("DISCORD_IPC_PATH");
    }

    const std::string test_client_id = "1462972195658534965";
    ReaCord::Discord::Client client;
    client.Start(test_client_id);

    bool connected = false;
    for (int i = 0; i < 40; ++i) { // up to 2 seconds
        if (client.IsConnected() && server.HandshakeReceived()) {
            connected = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    client.Stop();
    server.Stop();

    if (!connected) {
        std::cerr << "FAIL (client did not connect to socket at " << sock_path << ")\n";
        return false;
    }

    if (server.ReceivedClientId() != test_client_id) {
        std::cerr << "FAIL (client ID mismatch: got " << server.ReceivedClientId()
                  << ", expected " << test_client_id << ")\n";
        return false;
    }

    std::cout << "PASS\n";
    return true;
}

static bool RunFastFailTest(const std::string& empty_dir) {
    std::cout << "[TEST] FastFailNonExistent ... " << std::flush;
    setenv("XDG_RUNTIME_DIR", empty_dir.c_str(), 1);
    unsetenv("DISCORD_IPC_PATH");

    auto start = std::chrono::steady_clock::now();
    ReaCord::Discord::Client client;
    client.Start("1462972195658534965");

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    bool not_connected = !client.IsConnected();
    client.Stop();

    if (!not_connected) {
        std::cerr << "FAIL (connected to non-existent socket)\n";
        return false;
    }

    std::cout << "PASS (" << duration << "ms)\n";
    return true;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  ReaCord Linux IPC Socket Discovery Tests\n";
    std::cout << "========================================\n";

    char temp_template[] = "/tmp/reacord_test_XXXXXX";
    char* temp_path = mkdtemp(temp_template);
    if (!temp_path) {
        std::cerr << "Fatal: Failed to create temporary test directory\n";
        return 1;
    }
    g_test_root = temp_path;
    setenv("XDG_RUNTIME_DIR", g_test_root.c_str(), 1);

    bool all_passed = true;

    // 1. Standard native Linux Discord path ($XDG_RUNTIME_DIR/discord-ipc-0)
    all_passed &= RunConnectionTest("StandardNative_discord-ipc-0", "", 0);

    // 2. Official Flathub Flatpak Discord ($XDG_RUNTIME_DIR/app/com.discordapp.Discord/discord-ipc-0)
    all_passed &= RunConnectionTest("FlatpakOfficial_com.discordapp.Discord", "app/com.discordapp.Discord", 0);

    // 3. Flatpak Discord Canary ($XDG_RUNTIME_DIR/app/com.discordapp.DiscordCanary/discord-ipc-0)
    all_passed &= RunConnectionTest("FlatpakCanary_com.discordapp.DiscordCanary", "app/com.discordapp.DiscordCanary", 0);

    // 4. Flatpak Vesktop ($XDG_RUNTIME_DIR/app/dev.vencord.Vesktop/discord-ipc-0)
    all_passed &= RunConnectionTest("FlatpakVesktop_dev.vencord.Vesktop", "app/dev.vencord.Vesktop", 0);

    // 5. Dynamic scan for unlisted/custom Flatpak clients ($XDG_RUNTIME_DIR/app/custom.discord.client/discord-ipc-2)
    all_passed &= RunConnectionTest("FlatpakDynamicScan_custom.discord.client", "app/custom.discord.client", 2);

    // 6. Snap Discord ($XDG_RUNTIME_DIR/snap.discord/discord-ipc-0)
    all_passed &= RunConnectionTest("SnapDiscord_snap.discord", "snap.discord", 0);

    // 7. Manual override via DISCORD_IPC_PATH environment variable
    std::string custom_override = g_test_root + "/custom_manual_override";
    all_passed &= RunConnectionTest("CustomPathOverride_DISCORD_IPC_PATH", "custom_manual_override", 0, custom_override);

    // 8. Fast-fail test on empty directory (verifies sub-millisecond stat() behavior)
    std::string empty_dir = g_test_root + "/empty_dir";
    MkdirP(empty_dir);
    all_passed &= RunFastFailTest(empty_dir);

    // Cleanup
    RemoveDirRecursive(g_test_root);

    std::cout << "========================================\n";
    if (all_passed) {
        std::cout << "  ALL TESTS PASSED SUCCESSFULLY! (8/8)\n";
        std::cout << "========================================\n";
        return 0;
    } else {
        std::cerr << "  SOME TESTS FAILED!\n";
        std::cout << "========================================\n";
        return 1;
    }
}
#endif
