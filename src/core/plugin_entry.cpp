#define REAPERAPI_IMPLEMENT
#include "reaper/reaper_api.h"

#include "config.hpp"
#include "reaper/reaper_observer.hpp"
#include "discord/discord_ipc.hpp"
#include "ui/settings_dialog.hpp"

namespace ReaCord {

// Global Discord client instance
Discord::Client g_discord_client;
REAPER_PLUGIN_HINSTANCE g_hInstance = nullptr;

static int g_cmd_settings = 0;
static int g_cmd_settings_native = 0;
static int g_cmd_settings_reaimgui = 0;
static int g_cmd_incognito = 0;

typedef struct {
    ACCEL accel;
    const char* desc;
} gaccel_register_t;

static gaccel_register_t g_accel_settings = {
    { 0, 0, 0 },
    "ReaCord: Open Settings..."
};

static gaccel_register_t g_accel_settings_native = {
    { 0, 0, 0 },
    "ReaCord: Open Classic Win32 Settings"
};

static gaccel_register_t g_accel_settings_reaimgui = {
    { 0, 0, 0 },
    "ReaCord: Open Modern Settings (ReaImGui)"
};

static gaccel_register_t g_accel_incognito = {
    { 0, 0, 0 },
    "ReaCord: Toggle Incognito Mode"
};

static int g_open_classic_delay_ticks = -1;

static void ReaCord_TimerHook() {
    Observer::Instance().OnTimerTick();

    bool requested = UI::CheckAndResetRequestOpenClassic();
    if (!requested && GetExtState) {
        const char* req = GetExtState("ReaCord", "request_open_classic");
        if (req && strcmp(req, "1") == 0) {
            requested = true;
        }
    }

    if (requested) {
        if (DeleteExtState) {
            DeleteExtState("ReaCord", "request_open_classic", false);
        } else if (SetExtState) {
            SetExtState("ReaCord", "request_open_classic", "", false);
        }
        UI::HideReaImGuiWindow();
        g_open_classic_delay_ticks = 4; // Allow ~120ms for ReaImGui to garbage-collect and destroy its platform windows
    }

    if (g_open_classic_delay_ticks > 0) {
        --g_open_classic_delay_ticks;
        UI::HideReaImGuiWindow();
        if (g_open_classic_delay_ticks == 0) {
            g_open_classic_delay_ticks = -1;
            UI::ShowNativeSettingsDialog(g_hInstance, GetMainHwnd ? GetMainHwnd() : nullptr);
        }
    }
}

static bool ReaCord_HookCommand(int command, int /*flag*/) {
    if (command && command == g_cmd_settings) {
        UI::ShowSettingsDialog(g_hInstance, GetMainHwnd ? GetMainHwnd() : nullptr);
        return true;
    }
    if (command && command == g_cmd_settings_native) {
        UI::ShowNativeSettingsDialog(g_hInstance, GetMainHwnd ? GetMainHwnd() : nullptr);
        return true;
    }
    if (command && command == g_cmd_settings_reaimgui) {
        if (!UI::LaunchReaImGuiScript()) {
            UI::ShowSettingsDialog(g_hInstance, GetMainHwnd ? GetMainHwnd() : nullptr);
        }
        return true;
    }
    if (command && command == g_cmd_incognito) {
        Config& cfg = Config::Instance();
        cfg.incognito = !cfg.incognito;
        cfg.Save();
        return true;
    }
    return false;
}

static void ReaCord_MenuHook(const char* menuidstr, void* menu, int /*flag*/) {
    if (!menuidstr || strcmp(menuidstr, "Main extensions") != 0) return;
    if (!menu || !g_cmd_settings) return;

    HMENU hMenu = static_cast<HMENU>(menu);
    int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; ++i) {
        if (static_cast<int>(GetMenuItemID(hMenu, i)) == g_cmd_settings) {
            return; // Item already present
        }
    }

#ifdef _WIN32
    MENUITEMINFOA mi = { sizeof(MENUITEMINFOA) };
    mi.fMask = MIIM_TYPE | MIIM_ID;
    mi.fType = MFT_STRING;
    mi.wID = static_cast<UINT>(g_cmd_settings);
    mi.dwTypeData = const_cast<char*>("ReaCord Settings...");
    InsertMenuItemA(hMenu, count, TRUE, &mi);
#else
    MENUITEMINFO mi = { sizeof(MENUITEMINFO) };
    mi.fMask = MIIM_TYPE | MIIM_ID;
    mi.fType = MFT_STRING;
    mi.wID = static_cast<UINT>(g_cmd_settings);
    mi.dwTypeData = const_cast<char*>("ReaCord Settings...");
    InsertMenuItem(hMenu, count, TRUE, &mi);
#endif
}

// Forward declaration from api_export.cpp
void RegisterApiFunctions(reaper_plugin_info_t* rec);

} // namespace ReaCord

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
    REAPER_PLUGIN_HINSTANCE hInstance, 
    reaper_plugin_info_t *rec
) {
    ReaCord::g_hInstance = hInstance;

    if (!rec) {
        // REAPER is shutting down or unloading extension
        if (plugin_register) {
            plugin_register("-hookcustommenu", (void*)ReaCord::ReaCord_MenuHook);
            plugin_register("-timer", (void*)ReaCord::ReaCord_TimerHook);
            plugin_register("-hookcommand", (void*)ReaCord::ReaCord_HookCommand);
        }
        ReaCord::Observer::Instance().Shutdown();
        ReaCord::g_discord_client.Stop();
        return 0;
    }

    if (rec->caller_version != REAPER_PLUGIN_VERSION) {
        return 0;
    }

    // Load function pointers from REAPER
    if (REAPERAPI_LoadAPI(rec->GetFunc) != 0) {
        return 0;
    }

    // 1. Load persisted configuration
    ReaCord::Config::Instance().Load();

    // 2. Register Actions & Hotkeys
    ReaCord::g_cmd_settings = rec->Register("command_id", (void*)"REACORD_OPEN_SETTINGS");
    ReaCord::g_accel_settings.accel.cmd = static_cast<WORD>(ReaCord::g_cmd_settings);
    rec->Register("gaccel", &ReaCord::g_accel_settings);

    ReaCord::g_cmd_settings_native = rec->Register("command_id", (void*)"REACORD_OPEN_SETTINGS_NATIVE");
    ReaCord::g_accel_settings_native.accel.cmd = static_cast<WORD>(ReaCord::g_cmd_settings_native);
    rec->Register("gaccel", &ReaCord::g_accel_settings_native);

    ReaCord::g_cmd_settings_reaimgui = rec->Register("command_id", (void*)"REACORD_OPEN_SETTINGS_REAIMGUI");
    ReaCord::g_accel_settings_reaimgui.accel.cmd = static_cast<WORD>(ReaCord::g_cmd_settings_reaimgui);
    rec->Register("gaccel", &ReaCord::g_accel_settings_reaimgui);

    ReaCord::g_cmd_incognito = rec->Register("command_id", (void*)"REACORD_TOGGLE_INCOGNITO");
    ReaCord::g_accel_incognito.accel.cmd = static_cast<WORD>(ReaCord::g_cmd_incognito);
    rec->Register("gaccel", &ReaCord::g_accel_incognito);

    rec->Register("hookcommand", (void*)ReaCord::ReaCord_HookCommand);

    // Register Menu Hook and ensure Extensions main menu exists
    rec->Register("hookcustommenu", (void*)ReaCord::ReaCord_MenuHook);
    if (AddExtensionsMainMenu) {
        AddExtensionsMainMenu();
    }

    // 3. Register ReaScript C API exports
    ReaCord::RegisterApiFunctions(rec);

    // 4. Register Timer Hook for state polling (zero-overhead adaptive callback)
    rec->Register("timer", (void*)ReaCord::ReaCord_TimerHook);

    // 5. Start asynchronous Discord IPC Client & State Observer
    ReaCord::g_discord_client.Start(ReaCord::Config::Instance().client_id);
    ReaCord::Observer::Instance().Initialize(&ReaCord::g_discord_client);

    return 1; // Success
}
