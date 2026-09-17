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
static int g_cmd_incognito = 0;

typedef struct {
    ACCEL accel;
    const char* desc;
} gaccel_register_t;

static gaccel_register_t g_accel_settings = {
    { 0, 0, 0 },
    "ReaCord: Open Settings..."
};

static gaccel_register_t g_accel_incognito = {
    { 0, 0, 0 },
    "ReaCord: Toggle Incognito Mode"
};

static void ReaCord_TimerHook() {
    Observer::Instance().OnTimerTick();
}

static bool ReaCord_HookCommand(int command, int flag) {
    if (command && command == g_cmd_settings) {
        UI::ShowSettingsDialog(g_hInstance, GetMainHwnd ? GetMainHwnd() : nullptr);
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
    ReaCord::g_cmd_settings = static_cast<int>(reinterpret_cast<INT_PTR>(rec->Register("command_id", (void*)"REACORD_OPEN_SETTINGS")));
    ReaCord::g_accel_settings.accel.cmd = static_cast<WORD>(ReaCord::g_cmd_settings);
    rec->Register("gaccel", &ReaCord::g_accel_settings);

    ReaCord::g_cmd_incognito = static_cast<int>(reinterpret_cast<INT_PTR>(rec->Register("command_id", (void*)"REACORD_TOGGLE_INCOGNITO")));
    ReaCord::g_accel_incognito.accel.cmd = static_cast<WORD>(ReaCord::g_cmd_incognito);
    rec->Register("gaccel", &ReaCord::g_accel_incognito);

    rec->Register("hookcommand", (void*)ReaCord::ReaCord_HookCommand);

    // 3. Register ReaScript C API exports
    ReaCord::RegisterApiFunctions(rec);

    // 4. Register Timer Hook for state polling (zero-overhead adaptive callback)
    rec->Register("timer", (void*)ReaCord::ReaCord_TimerHook);

    // 5. Start asynchronous Discord IPC Client & State Observer
    ReaCord::g_discord_client.Start(ReaCord::Config::Instance().client_id);
    ReaCord::Observer::Instance().Initialize(&ReaCord::g_discord_client);

    return 1; // Success
}
