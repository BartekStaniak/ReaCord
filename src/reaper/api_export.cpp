#include "core/config.hpp"
#include "discord/discord_ipc.hpp"
#include "reaper/reaper_api.h"
#include <cstdlib>
#include <cstring>
#include <string>

namespace ReaCord {

extern Discord::Client g_discord_client;

// ReaScript API: ReaCord_GetVersion
const char* API_ReaCord_GetVersion() {
    return "1.0.2";
}

// ReaScript API: ReaCord_GetStatus
const char* API_ReaCord_GetStatus() {
    thread_local std::string status_buf;
    status_buf = g_discord_client.GetStatusString();
    return status_buf.c_str();
}

// ReaScript API: ReaCord_GetConfig
const char* API_ReaCord_GetConfig(const char* key) {
    thread_local std::string val_buf;
    if (!key) return "";

    Config& cfg = Config::Instance();
    if (strcmp(key, "enabled") == 0) val_buf = cfg.enabled ? "1" : "0";
    else if (strcmp(key, "incognito") == 0) val_buf = cfg.incognito ? "1" : "0";
    else if (strcmp(key, "project_name_mode") == 0) val_buf = std::to_string(static_cast<int>(cfg.project_name_mode));
    else if (strcmp(key, "session_time_mode") == 0) val_buf = std::to_string(static_cast<int>(cfg.session_time_mode));
    else if (strcmp(key, "play_state_mode") == 0) val_buf = std::to_string(static_cast<int>(cfg.play_state_mode));
    else if (strcmp(key, "icon_style") == 0) val_buf = std::to_string(static_cast<int>(cfg.icon_style));
    else if (strcmp(key, "show_track_count") == 0) val_buf = cfg.show_track_count ? "1" : "0";
    else if (strcmp(key, "client_id") == 0) val_buf = cfg.client_id;
    else if (strcmp(key, "large_image_key") == 0) val_buf = cfg.GetEffectiveLargeImageKey();
    else val_buf = "";

    return val_buf.c_str();
}

// ReaScript API: ReaCord_SetConfig
bool API_ReaCord_SetConfig(const char* key, const char* val) {
    if (!key || !val) return false;

    Config& cfg = Config::Instance();
    if (strcmp(key, "enabled") == 0) cfg.enabled = (strcmp(val, "1") == 0);
    else if (strcmp(key, "incognito") == 0) cfg.incognito = (strcmp(val, "1") == 0);
    else if (strcmp(key, "project_name_mode") == 0) cfg.project_name_mode = static_cast<ProjectNameMode>(std::atoi(val));
    else if (strcmp(key, "session_time_mode") == 0) cfg.session_time_mode = static_cast<SessionTimeMode>(std::atoi(val));
    else if (strcmp(key, "play_state_mode") == 0) cfg.play_state_mode = static_cast<PlayStateMode>(std::atoi(val));
    else if (strcmp(key, "icon_style") == 0) {
        cfg.icon_style = static_cast<IconStyle>(std::atoi(val));
        cfg.large_image_key = (cfg.icon_style == IconStyle::ReaCordHybrid) ? "reacord_logo" : "reaper_logo";
    }
    else if (strcmp(key, "show_track_count") == 0) cfg.show_track_count = (strcmp(val, "1") == 0);
    else if (strcmp(key, "client_id") == 0) {
        cfg.client_id = val;
        g_discord_client.SetClientId(cfg.client_id);
    }
    else if (strcmp(key, "large_image_key") == 0) {
        cfg.large_image_key = val;
        if (cfg.large_image_key == "reacord_logo") cfg.icon_style = IconStyle::ReaCordHybrid;
        else if (cfg.large_image_key == "reaper_logo") cfg.icon_style = IconStyle::ReaperClassic;
    }
    else return false;

    cfg.Save();
    return true;
}

// ReaScript API: ReaCord_ToggleIncognito
bool API_ReaCord_ToggleIncognito() {
    Config& cfg = Config::Instance();
    cfg.incognito = !cfg.incognito;
    cfg.Save();
    return cfg.incognito;
}

void RegisterApiFunctions(reaper_plugin_info_t* rec) {
    if (!rec || !rec->Register) return;

    rec->Register("API_ReaCord_GetVersion", (void*)API_ReaCord_GetVersion);
    rec->Register("APIvararg_ReaCord_GetVersion", (void*)API_ReaCord_GetVersion);
    rec->Register("APIdef_ReaCord_GetVersion", (void*)"const char*\0\0\0Returns ReaCord version string");

    rec->Register("API_ReaCord_GetStatus", (void*)API_ReaCord_GetStatus);
    rec->Register("APIvararg_ReaCord_GetStatus", (void*)API_ReaCord_GetStatus);
    rec->Register("APIdef_ReaCord_GetStatus", (void*)"const char*\0\0\0Returns ReaCord Discord connection status (Connected, Connecting, Disconnected)");

    rec->Register("API_ReaCord_GetConfig", (void*)API_ReaCord_GetConfig);
    rec->Register("APIvararg_ReaCord_GetConfig", (void*)API_ReaCord_GetConfig);
    rec->Register("APIdef_ReaCord_GetConfig", (void*)"const char*\0const char*\0key\0Gets a ReaCord configuration value");

    rec->Register("API_ReaCord_SetConfig", (void*)API_ReaCord_SetConfig);
    rec->Register("APIvararg_ReaCord_SetConfig", (void*)API_ReaCord_SetConfig);
    rec->Register("APIdef_ReaCord_SetConfig", (void*)"bool\0const char*,const char*\0key,val\0Sets a ReaCord configuration value");

    rec->Register("API_ReaCord_ToggleIncognito", (void*)API_ReaCord_ToggleIncognito);
    rec->Register("APIvararg_ReaCord_ToggleIncognito", (void*)API_ReaCord_ToggleIncognito);
    rec->Register("APIdef_ReaCord_ToggleIncognito", (void*)"bool\0\0\0Toggles ReaCord incognito privacy mode");
}

} // namespace ReaCord
