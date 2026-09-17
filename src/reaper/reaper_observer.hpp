#pragma once

#include "core/config.hpp"
#include "discord/discord_ipc.hpp"
#include <string>

namespace ReaCord {

class Observer {
public:
    static Observer& Instance() {
        static Observer s_instance;
        return s_instance;
    }

    void Initialize(Discord::Client* client);
    void Shutdown();

    // Timer hook called by REAPER's main thread
    void OnTimerTick();

private:
    Observer() = default;

    Discord::Client* discord_client_ = nullptr;
    double last_poll_time_ = 0.0;
    int64_t app_start_time_ = 0;
    int64_t project_start_time_ = 0;
    std::string last_project_name_;
};

} // namespace ReaCord
