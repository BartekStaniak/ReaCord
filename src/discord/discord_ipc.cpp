#include "discord_ipc.hpp"
#include <iostream>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace ReaCord {
namespace Discord {

Client::Client() {
#ifdef _WIN32
    current_pid_ = static_cast<uint32_t>(GetCurrentProcessId());
#else
    current_pid_ = static_cast<uint32_t>(getpid());
#endif
}

Client::~Client() {
    Stop();
}

void Client::Start(const std::string& client_id) {
    if (is_running_.load()) {
        if (client_id_ == client_id) return;
        Stop();
    }

    client_id_ = client_id;
    is_running_.store(true);
    worker_thread_ = std::thread(&Client::WorkerLoop, this);
}

void Client::Stop() {
    if (!is_running_.load()) return;

    is_running_.store(false);
    cv_.notify_all();

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    Disconnect();
}

void Client::SetClientId(const std::string& client_id) {
    if (client_id_ == client_id) return;
    Start(client_id);
}

void Client::UpdateActivity(const Activity& activity) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    pending_activity_ = activity;
    has_pending_activity_ = true;
    cv_.notify_one();
}

void Client::ClearActivity() {
    Activity empty_act;
    empty_act.is_active = false;
    UpdateActivity(empty_act);
}

std::string Client::GetStatusString() const {
    switch (status_.load()) {
        case ConnectionStatus::Connected:
            return "Connected";
        case ConnectionStatus::Connecting:
            return "Connecting...";
        case ConnectionStatus::Disconnected:
        default:
            return "Disconnected";
    }
}

void Client::WorkerLoop() {
    auto last_connect_attempt = std::chrono::steady_clock::now() - std::chrono::seconds(30);

    while (is_running_.load()) {
        // 1. Connection management
        if (status_.load() != ConnectionStatus::Connected) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_connect_attempt).count();

            if (elapsed >= 5) {
                last_connect_attempt = now;
                status_.store(ConnectionStatus::Connecting);
                if (TryConnect()) {
                    status_.store(ConnectionStatus::Connected);
                    rate_limiter_.Reset();
                    force_next_send_ = true;
                } else {
                    status_.store(ConnectionStatus::Disconnected);
                }
            }
        }

        // 2. Read incoming responses / keepalive
        if (status_.load() == ConnectionStatus::Connected) {
            ReadIncoming();
        }

        // 3. Process activity updates
        if (status_.load() == ConnectionStatus::Connected) {
            Activity act_to_send;
            bool should_send = false;

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (has_pending_activity_) {
                    act_to_send = pending_activity_;
                    if (rate_limiter_.ShouldSend(act_to_send, force_next_send_)) {
                        should_send = true;
                        force_next_send_ = false;
                    }
                }
            }

            if (should_send) {
                static uint64_t nonce_counter = 0;
                std::string nonce = "reacord_" + std::to_string(++nonce_counter);
                std::string payload = BuildSetActivityPayload(current_pid_, act_to_send, nonce);

                if (!SendPacket(Opcode::Frame, payload)) {
                    Disconnect();
                    status_.store(ConnectionStatus::Disconnected);
                }
            }
        }

        // 4. Wait on condition variable or timeout
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(500), [this] {
            return !is_running_.load();
        });
    }

    // Cleanup before thread exit
    if (status_.load() == ConnectionStatus::Connected) {
        // Send clear activity
        Activity clear_act;
        clear_act.is_active = false;
        std::string payload = BuildSetActivityPayload(current_pid_, clear_act, "reacord_close");
        SendPacket(Opcode::Frame, payload);
        SendPacket(Opcode::Close, "{}");
    }
}

#ifdef _WIN32

bool Client::TryConnect() {
    Disconnect();

    if (client_id_.empty()) return false;

    // Try pipes 0 through 9
    for (int i = 0; i < 10; ++i) {
        std::string pipe_name = "\\\\.\\pipe\\discord-ipc-" + std::to_string(i);
        HANDLE h = CreateFileA(
            pipe_name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
            nullptr
        );

        if (h != INVALID_HANDLE_VALUE) {
            pipe_handle_ = h;

            // Perform handshake
            std::string handshake = BuildHandshakePayload(client_id_);
            if (SendPacket(Opcode::Handshake, handshake)) {
                return true;
            }

            CloseHandle(h);
            pipe_handle_ = nullptr;
        }
    }

    return false;
}

void Client::Disconnect() {
    if (pipe_handle_ && pipe_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(static_cast<HANDLE>(pipe_handle_));
        pipe_handle_ = nullptr;
    }
}

bool Client::SendPacket(Opcode opcode, const std::string& payload) {
    if (!pipe_handle_ || pipe_handle_ == INVALID_HANDLE_VALUE) return false;

    std::vector<uint8_t> msg = PackMessage(opcode, payload);

    OVERLAPPED ov = {0};
    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) return false;

    DWORD bytes_written = 0;
    BOOL res = WriteFile(
        static_cast<HANDLE>(pipe_handle_),
        msg.data(),
        static_cast<DWORD>(msg.size()),
        &bytes_written,
        &ov
    );

    bool success = false;
    if (!res) {
        if (GetLastError() == ERROR_IO_PENDING) {
            DWORD wait_res = WaitForSingleObject(ov.hEvent, 1000);
            if (wait_res == WAIT_OBJECT_0) {
                success = (GetOverlappedResult(static_cast<HANDLE>(pipe_handle_), &ov, &bytes_written, FALSE) != 0);
            }
        }
    } else {
        success = true;
    }

    CloseHandle(ov.hEvent);
    return success;
}

void Client::ReadIncoming() {
    if (!pipe_handle_ || pipe_handle_ == INVALID_HANDLE_VALUE) return;

    DWORD bytes_avail = 0;
    if (PeekNamedPipe(static_cast<HANDLE>(pipe_handle_), nullptr, 0, nullptr, &bytes_avail, nullptr) && bytes_avail > 0) {
        // Clamp maximum buffer size to 64 KB to prevent malicious allocation exhaustion
        DWORD to_read = (bytes_avail > 65536) ? 65536 : bytes_avail;
        std::vector<uint8_t> buf(to_read);
        OVERLAPPED ov = {0};
        ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (ov.hEvent) {
            DWORD bytes_read = 0;
            if (ReadFile(static_cast<HANDLE>(pipe_handle_), buf.data(), to_read, &bytes_read, &ov) ||
                GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(ov.hEvent, 100);
            }
            CloseHandle(ov.hEvent);
        }
    }
}

#else // macOS and Linux (POSIX)

static const char* GetSocketDirectory() {
    const char* dir = getenv("XDG_RUNTIME_DIR");
    if (dir && dir[0]) return dir;
    dir = getenv("TMPDIR");
    if (dir && dir[0]) return dir;
    dir = getenv("TMP");
    if (dir && dir[0]) return dir;
    dir = getenv("TEMP");
    if (dir && dir[0]) return dir;
    return "/tmp";
}

bool Client::TryConnect() {
    Disconnect();

    if (client_id_.empty()) return false;

    const char* base_dir = GetSocketDirectory();

    for (int i = 0; i < 10; ++i) {
        std::string sock_path = std::string(base_dir) + "/discord-ipc-" + std::to_string(i);

        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) continue;

        // Set non-blocking
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        if (sock_path.length() >= sizeof(addr.sun_path)) {
            close(fd);
            continue;
        }
        strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

        int res = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        bool connected = false;

        if (res == 0) {
            connected = true;
        } else if (errno == EINPROGRESS) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(fd, &set);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 250000; // 250ms

            if (select(fd + 1, nullptr, &set, nullptr, &tv) > 0) {
                int err = 0;
                socklen_t len = sizeof(err);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (err == 0) connected = true;
            }
        }

        if (connected) {
            socket_fd_ = fd;
            std::string handshake = BuildHandshakePayload(client_id_);
            if (SendPacket(Opcode::Handshake, handshake)) {
                return true;
            }
            close(fd);
            socket_fd_ = -1;
        } else {
            close(fd);
        }
    }

    return false;
}

void Client::Disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool Client::SendPacket(Opcode opcode, const std::string& payload) {
    if (socket_fd_ < 0) return false;

    std::vector<uint8_t> msg = PackMessage(opcode, payload);
    size_t total_sent = 0;

    while (total_sent < msg.size()) {
#ifdef __APPLE__
        ssize_t sent = send(socket_fd_, msg.data() + total_sent, msg.size() - total_sent, 0);
#else
        ssize_t sent = send(socket_fd_, msg.data() + total_sent, msg.size() - total_sent, MSG_NOSIGNAL);
#endif
        if (sent > 0) {
            total_sent += sent;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
            }
            return false;
        }
    }

    return true;
}

void Client::ReadIncoming() {
    if (socket_fd_ < 0) return;

    char buf[1024];
    while (true) {
        ssize_t n = recv(socket_fd_, buf, sizeof(buf), 0);
        if (n <= 0) break;
    }
}

#endif

} // namespace Discord
} // namespace ReaCord
