#include "core/config.hpp"
#include "discord/discord_ipc.hpp"
#include "reaper/reaper_api.h"
#include "reaper_observer.hpp"
#include "ui/settings_dialog.hpp"
#include <cstdlib>
#include <cstring>
#include <string>

namespace ReaCord {

extern Discord::Client g_discord_client;

// ReaScript API: ReaCord_GetVersion
const char* API_ReaCord_GetVersion() {
    return REACORD_VERSION;
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
    else if (strcmp(key, "extstate_section") == 0) val_buf = cfg.extstate_section;
    else if (strcmp(key, "extstate_key") == 0) val_buf = cfg.extstate_key;
    else if (strcmp(key, "extstate_in_state_text") == 0) val_buf = cfg.extstate_in_state_text ? "1" : "0";
    else if (strcmp(key, "prefer_reaimgui") == 0) val_buf = cfg.prefer_reaimgui ? "1" : "0";
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
    else if (strcmp(key, "extstate_section") == 0) cfg.extstate_section = val;
    else if (strcmp(key, "extstate_key") == 0) cfg.extstate_key = val;
    else if (strcmp(key, "extstate_in_state_text") == 0) cfg.extstate_in_state_text = (strcmp(val, "1") == 0);
    else if (strcmp(key, "prefer_reaimgui") == 0) cfg.prefer_reaimgui = (strcmp(val, "1") == 0);
    else return false;

    cfg.Save();
    Observer::Instance().TriggerInstantUpdate();
    return true;
}

// ReaScript API: ReaCord_ToggleIncognito
bool API_ReaCord_ToggleIncognito() {
    Config& cfg = Config::Instance();
    cfg.incognito = !cfg.incognito;
    cfg.Save();
    Observer::Instance().TriggerInstantUpdate();
    return cfg.incognito;
}

// ReaScript API: ReaCord_SwitchToClassicUI
bool API_ReaCord_SwitchToClassicUI() {
    UI::RequestOpenClassicDialog();
    return true;
}

// ReaScript API: ReaCord_TriggerUpdate
bool API_ReaCord_TriggerUpdate() {
    Config::Instance().Load();
    g_discord_client.SetClientId(Config::Instance().client_id);
    Observer::Instance().TriggerInstantUpdate();
    return true;
}

// ReaScript vararg unpacking wrappers: (void* (*)(void** arglist, int numparms))
static void* APIvararg_ReaCord_GetVersion(void** /*arglist*/, int /*numparms*/) {
    return (void*)API_ReaCord_GetVersion();
}

static void* APIvararg_ReaCord_GetStatus(void** /*arglist*/, int /*numparms*/) {
    return (void*)API_ReaCord_GetStatus();
}

static void* APIvararg_ReaCord_GetConfig(void** arglist, int numparms) {
    if (!arglist || numparms < 1) return (void*)"";
    const char* key = static_cast<const char*>(arglist[0]);
    return (void*)API_ReaCord_GetConfig(key);
}

static void* APIvararg_ReaCord_SetConfig(void** arglist, int numparms) {
    if (!arglist || numparms < 2) return (void*)(INT_PTR)0;
    const char* key = static_cast<const char*>(arglist[0]);
    const char* val = static_cast<const char*>(arglist[1]);
    bool res = API_ReaCord_SetConfig(key, val);
    return (void*)(INT_PTR)(res ? 1 : 0);
}

static void* APIvararg_ReaCord_ToggleIncognito(void** /*arglist*/, int /*numparms*/) {
    bool res = API_ReaCord_ToggleIncognito();
    return (void*)(INT_PTR)(res ? 1 : 0);
}

static void* APIvararg_ReaCord_SwitchToClassicUI(void** /*arglist*/, int /*numparms*/) {
    bool res = API_ReaCord_SwitchToClassicUI();
    return (void*)(INT_PTR)(res ? 1 : 0);
}

static void* APIvararg_ReaCord_TriggerUpdate(void** /*arglist*/, int /*numparms*/) {
    bool res = API_ReaCord_TriggerUpdate();
    return (void*)(INT_PTR)(res ? 1 : 0);
}

void RegisterApiFunctions(reaper_plugin_info_t* rec) {
    if (!rec || !rec->Register) return;

    rec->Register("API_ReaCord_GetVersion", (void*)API_ReaCord_GetVersion);
    rec->Register("APIvararg_ReaCord_GetVersion", (void*)APIvararg_ReaCord_GetVersion);
    rec->Register("APIdef_ReaCord_GetVersion", (void*)"const char*\0\0\0Returns ReaCord version string");

    rec->Register("API_ReaCord_GetStatus", (void*)API_ReaCord_GetStatus);
    rec->Register("APIvararg_ReaCord_GetStatus", (void*)APIvararg_ReaCord_GetStatus);
    rec->Register("APIdef_ReaCord_GetStatus", (void*)"const char*\0\0\0Returns ReaCord Discord connection status (Connected, Connecting, Disconnected)");

    rec->Register("API_ReaCord_GetConfig", (void*)API_ReaCord_GetConfig);
    rec->Register("APIvararg_ReaCord_GetConfig", (void*)APIvararg_ReaCord_GetConfig);
    rec->Register("APIdef_ReaCord_GetConfig", (void*)"const char*\0const char*\0key\0Gets a ReaCord configuration value");

    rec->Register("API_ReaCord_SetConfig", (void*)API_ReaCord_SetConfig);
    rec->Register("APIvararg_ReaCord_SetConfig", (void*)APIvararg_ReaCord_SetConfig);
    rec->Register("APIdef_ReaCord_SetConfig", (void*)"bool\0const char*,const char*\0key,val\0Sets a ReaCord configuration value");

    rec->Register("API_ReaCord_ToggleIncognito", (void*)API_ReaCord_ToggleIncognito);
    rec->Register("APIvararg_ReaCord_ToggleIncognito", (void*)APIvararg_ReaCord_ToggleIncognito);
    rec->Register("APIdef_ReaCord_ToggleIncognito", (void*)"bool\0\0\0Toggles ReaCord incognito privacy mode");

    rec->Register("API_ReaCord_SwitchToClassicUI", (void*)API_ReaCord_SwitchToClassicUI);
    rec->Register("APIvararg_ReaCord_SwitchToClassicUI", (void*)APIvararg_ReaCord_SwitchToClassicUI);
    rec->Register("APIdef_ReaCord_SwitchToClassicUI", (void*)"bool\0\0\0Requests REAPER to open the classic native ReaCord settings dialog on the next main loop tick");

    rec->Register("API_ReaCord_TriggerUpdate", (void*)API_ReaCord_TriggerUpdate);
    rec->Register("APIvararg_ReaCord_TriggerUpdate", (void*)APIvararg_ReaCord_TriggerUpdate);
    rec->Register("APIdef_ReaCord_TriggerUpdate", (void*)"bool\0\0\0Triggers an immediate Discord Rich Presence update");
}

} // namespace ReaCord
