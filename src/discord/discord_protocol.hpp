#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdio>

namespace ReaCord {
namespace Discord {

enum class Opcode : uint32_t {
    Handshake = 0,
    Frame = 1,
    Close = 2,
    Ping = 3,
    Pong = 4
};

// Activity data model
struct Activity {
    bool is_active = false;
    std::string details;       // Line 1: e.g. "Project: TrackName.rpp"
    std::string state;         // Line 2: e.g. "Playing (128 BPM)"
    int64_t start_time = 0;    // Epoch seconds (0 to omit)
    int64_t end_time = 0;      // Epoch seconds (0 to omit)
    std::string large_image;   // Asset key e.g. "reaper_logo"
    std::string large_text;    // Hover text e.g. "REAPER v7.x"
    std::string small_image;   // Asset key e.g. "play", "record", "pause"
    std::string small_text;    // Hover text e.g. "Recording"
};

// Utility to escape JSON strings safely
inline std::string EscapeJsonString(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (char c : input) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

// Build Handshake Packet
inline std::string BuildHandshakePayload(const std::string& client_id) {
    std::ostringstream ss;
    ss << "{\"v\":1,\"client_id\":\"" << EscapeJsonString(client_id) << "\"}";
    return ss.str();
}

// Build SET_ACTIVITY Packet
inline std::string BuildSetActivityPayload(uint32_t pid, const Activity& act, const std::string& nonce) {
    std::ostringstream ss;
    ss << "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":" << pid << ",\"activity\":";

    if (!act.is_active) {
        ss << "null";
    } else {
        ss << "{";
        bool has_field = false;

        if (!act.details.empty()) {
            ss << "\"details\":\"" << EscapeJsonString(act.details) << "\"";
            has_field = true;
        }

        if (!act.state.empty()) {
            if (has_field) ss << ",";
            ss << "\"state\":\"" << EscapeJsonString(act.state) << "\"";
            has_field = true;
        }

        if (act.start_time > 0 || act.end_time > 0) {
            if (has_field) ss << ",";
            ss << "\"timestamps\":{";
            bool has_ts = false;
            if (act.start_time > 0) {
                ss << "\"start\":" << act.start_time;
                has_ts = true;
            }
            if (act.end_time > 0) {
                if (has_ts) ss << ",";
                ss << "\"end\":" << act.end_time;
            }
            ss << "}";
            has_field = true;
        }

        if (!act.large_image.empty() || !act.small_image.empty()) {
            if (has_field) ss << ",";
            ss << "\"assets\":{";
            bool has_asset = false;
            if (!act.large_image.empty()) {
                ss << "\"large_image\":\"" << EscapeJsonString(act.large_image) << "\"";
                if (!act.large_text.empty()) {
                    ss << ",\"large_text\":\"" << EscapeJsonString(act.large_text) << "\"";
                }
                has_asset = true;
            }
            if (!act.small_image.empty()) {
                if (has_asset) ss << ",";
                ss << "\"small_image\":\"" << EscapeJsonString(act.small_image) << "\"";
                if (!act.small_text.empty()) {
                    ss << ",\"small_text\":\"" << EscapeJsonString(act.small_text) << "\"";
                }
            }
            ss << "}";
        }

        ss << "}";
    }

    ss << "},\"nonce\":\"" << EscapeJsonString(nonce) << "\"}";
    return ss.str();
}

// Encode packet with 8-byte Discord header
inline std::vector<uint8_t> PackMessage(Opcode opcode, const std::string& payload) {
    uint32_t op = static_cast<uint32_t>(opcode);
    uint32_t len = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> buffer(8 + len);
    // Little-endian opcode
    buffer[0] = static_cast<uint8_t>(op & 0xFF);
    buffer[1] = static_cast<uint8_t>((op >> 8) & 0xFF);
    buffer[2] = static_cast<uint8_t>((op >> 16) & 0xFF);
    buffer[3] = static_cast<uint8_t>((op >> 24) & 0xFF);

    // Little-endian length
    buffer[4] = static_cast<uint8_t>(len & 0xFF);
    buffer[5] = static_cast<uint8_t>((len >> 8) & 0xFF);
    buffer[6] = static_cast<uint8_t>((len >> 16) & 0xFF);
    buffer[7] = static_cast<uint8_t>((len >> 24) & 0xFF);

    // Payload
    if (len > 0) {
        std::memcpy(buffer.data() + 8, payload.data(), len);
    }

    return buffer;
}

} // namespace Discord
} // namespace ReaCord
