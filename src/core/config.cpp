#include "config.hpp"
#include "reaper/reaper_api.h"
#include <cstdlib>
#include <cstring>

namespace ReaCord {

static const char* EXT_SECTION = "ReaCord";

Config& Config::Instance() {
    static Config s_config;
    return s_config;
}

static std::string ReadExt(const char* key, const std::string& default_val) {
    if (GetExtState) {
        const char* val = GetExtState(EXT_SECTION, key);
        if (val && val[0]) {
            return std::string(val);
        }
    }
    return default_val;
}

static void WriteExt(const char* key, const std::string& val) {
    if (SetExtState) {
        SetExtState(EXT_SECTION, key, val.c_str(), true); // Persist across sessions
    }
}

void Config::Load() {
    enabled = (ReadExt("enabled", "1") == "1");
    incognito = (ReadExt("incognito", "0") == "1");
    project_name_mode = static_cast<ProjectNameMode>(std::atoi(ReadExt("project_name_mode", "1").c_str()));
    session_time_mode = static_cast<SessionTimeMode>(std::atoi(ReadExt("session_time_mode", "1").c_str()));
    play_state_mode = static_cast<PlayStateMode>(std::atoi(ReadExt("play_state_mode", "2").c_str()));
    show_track_count = (ReadExt("show_track_count", "1") == "1");
    client_id = ReadExt("client_id", "123456789012345678");
    large_image_key = ReadExt("large_image_key", "reaper_logo");
    idle_timeout_mins = std::atoi(ReadExt("idle_timeout_mins", "15").c_str());
}

void Config::Save() const {
    WriteExt("enabled", enabled ? "1" : "0");
    WriteExt("incognito", incognito ? "1" : "0");
    WriteExt("project_name_mode", std::to_string(static_cast<int>(project_name_mode)));
    WriteExt("session_time_mode", std::to_string(static_cast<int>(session_time_mode)));
    WriteExt("play_state_mode", std::to_string(static_cast<int>(play_state_mode)));
    WriteExt("show_track_count", show_track_count ? "1" : "0");
    WriteExt("client_id", client_id);
    WriteExt("large_image_key", large_image_key);
    WriteExt("idle_timeout_mins", std::to_string(idle_timeout_mins));
}

} // namespace ReaCord
