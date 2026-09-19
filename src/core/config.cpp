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

static bool IsValidClientId(const std::string& id) {
    if (id.empty() || id.length() < 17 || id.length() > 22) return false;
    for (char c : id) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

void Config::Load() {
    enabled = (ReadExt("enabled", "1") == "1");
    incognito = (ReadExt("incognito", "0") == "1");
    project_name_mode = static_cast<ProjectNameMode>(std::atoi(ReadExt("project_name_mode", "1").c_str()));
    session_time_mode = static_cast<SessionTimeMode>(std::atoi(ReadExt("session_time_mode", "1").c_str()));
    play_state_mode = static_cast<PlayStateMode>(std::atoi(ReadExt("play_state_mode", "2").c_str()));
    show_track_count = (ReadExt("show_track_count", "1") == "1");
    client_id = ReadExt("client_id", REACORD_DEFAULT_CLIENT_ID);
    if (!IsValidClientId(client_id) || client_id == "123456789012345678") {
        client_id = REACORD_DEFAULT_CLIENT_ID;
    }
    large_image_key = ReadExt("large_image_key", "");
    icon_style = static_cast<IconStyle>(std::atoi(ReadExt("icon_style", "0").c_str()));
    if (large_image_key == "reacord_logo") {
        icon_style = IconStyle::ReaCordHybrid;
    } else if (large_image_key == "reaper_logo") {
        icon_style = IconStyle::ReaperClassic;
    } else if (large_image_key.empty()) {
        large_image_key = (icon_style == IconStyle::ReaCordHybrid) ? "reacord_logo" : "reaper_logo";
    }
    extstate_section = ReadExt("extstate_section", "PROJECT_TIME");
    extstate_key = ReadExt("extstate_key", "active_time");
    extstate_in_state_text = (ReadExt("extstate_in_state_text", "0") == "1");
    idle_timeout_mins = std::atoi(ReadExt("idle_timeout_mins", "15").c_str());
    prefer_reaimgui = (ReadExt("prefer_reaimgui", "0") == "1");
}

void Config::Save() const {
    WriteExt("enabled", enabled ? "1" : "0");
    WriteExt("incognito", incognito ? "1" : "0");
    WriteExt("project_name_mode", std::to_string(static_cast<int>(project_name_mode)));
    WriteExt("session_time_mode", std::to_string(static_cast<int>(session_time_mode)));
    WriteExt("play_state_mode", std::to_string(static_cast<int>(play_state_mode)));
    WriteExt("icon_style", std::to_string(static_cast<int>(icon_style)));
    WriteExt("show_track_count", show_track_count ? "1" : "0");
    WriteExt("client_id", client_id);
    WriteExt("large_image_key", GetEffectiveLargeImageKey());
    WriteExt("extstate_section", extstate_section);
    WriteExt("extstate_key", extstate_key);
    WriteExt("extstate_in_state_text", extstate_in_state_text ? "1" : "0");
    WriteExt("idle_timeout_mins", std::to_string(idle_timeout_mins));
    WriteExt("prefer_reaimgui", prefer_reaimgui ? "1" : "0");
}

} // namespace ReaCord
