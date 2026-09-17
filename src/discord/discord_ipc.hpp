#pragma once

#include "discord_protocol.hpp"
#include "rate_limiter.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace ReaCord {
namespace Discord {

enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected
};

class Client {
public:
    Client();
    ~Client();

    // Start background IPC thread
    void Start(const std::string& client_id);

    // Stop and disconnect cleanly
    void Stop();

    // Update presence (asynchronous, thread-safe)
    void UpdateActivity(const Activity& activity);

    // Clear presence
    void ClearActivity();

    // Update client ID (reconnects if changed)
    void SetClientId(const std::string& client_id);

    // Status queries
    bool IsConnected() const { return status_.load() == ConnectionStatus::Connected; }
    ConnectionStatus GetStatus() const { return status_.load(); }
    std::string GetStatusString() const;

private:
    void WorkerLoop();
    bool TryConnect();
    void Disconnect();
    bool SendPacket(Opcode opcode, const std::string& payload);
    void ReadIncoming();

#ifdef _WIN32
    void* pipe_handle_ = nullptr; // HANDLE
#else
    int socket_fd_ = -1;
#endif

    std::string client_id_;
    std::atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
    std::atomic<bool> is_running_{false};

    std::thread worker_thread_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;

    Activity pending_activity_;
    bool has_pending_activity_ = false;
    bool force_next_send_ = false;

    RateLimiter rate_limiter_;
    uint32_t current_pid_ = 0;
};

} // namespace Discord
} // namespace ReaCord
