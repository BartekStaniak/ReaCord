#pragma once

#include "discord_protocol.hpp"
#include <chrono>
#include <cstdint>

namespace ReaCord {
namespace Discord {

class RateLimiter {
public:
    RateLimiter() 
        : last_send_time_(std::chrono::steady_clock::now() - std::chrono::seconds(60)),
          last_hash_(0) {}

    // 64-bit FNV-1a Hash of activity content
    static uint64_t HashActivity(const Activity& act) {
        uint64_t hash = 14695981039346656037ULL;
        auto hash_str = [&hash](const std::string& str) {
            for (char c : str) {
                hash ^= static_cast<uint8_t>(c);
                hash *= 1099511628211ULL;
            }
        };
        auto hash_u64 = [&hash](uint64_t val) {
            for (int i = 0; i < 8; ++i) {
                hash ^= static_cast<uint8_t>(val & 0xFF);
                hash *= 1099511628211ULL;
                val >>= 8;
            }
        };

        hash_u64(act.is_active ? 1 : 0);
        hash_str(act.details);
        hash_str(act.state);
        hash_u64(static_cast<uint64_t>(act.start_time));
        hash_str(act.large_image);
        hash_str(act.large_text);
        hash_str(act.small_image);
        hash_str(act.small_text);

        return hash;
    }

    // Evaluates whether an activity frame should be transmitted
    bool ShouldSend(const Activity& act, bool force_send = false) {
        uint64_t current_hash = HashActivity(act);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_send_time_).count();

        // 1. Force send override
        if (force_send) {
            last_hash_ = current_hash;
            last_send_time_ = now;
            return true;
        }

        // 2. If nothing changed, don't send anything
        if (current_hash == last_hash_) {
            return false;
        }

        // 3. Rate limiting: Minimum 2000ms (2s) between updates
        if (elapsed < 2000) {
            return false;
        }

        // 4. Update internal state
        last_hash_ = current_hash;
        last_send_time_ = now;
        return true;
    }

    void Reset() {
        last_hash_ = 0;
        last_send_time_ = std::chrono::steady_clock::now() - std::chrono::seconds(60);
    }

private:
    std::chrono::steady_clock::time_point last_send_time_;
    uint64_t last_hash_;
};

} // namespace Discord
} // namespace ReaCord
