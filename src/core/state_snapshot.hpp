#pragma once

#include "discord/discord_protocol.hpp"
#include <mutex>

namespace ReaCord {

class StateStore {
public:
    static StateStore& Instance() {
        static StateStore s_instance;
        return s_instance;
    }

    void SetActivity(const Discord::Activity& act) {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_ = act;
    }

    Discord::Activity GetActivity() {
        std::lock_guard<std::mutex> lock(mutex_);
        return snapshot_;
    }

private:
    StateStore() = default;
    std::mutex mutex_;
    Discord::Activity snapshot_;
};

} // namespace ReaCord
