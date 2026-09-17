#include "settings_dialog.hpp"
#include "core/config.hpp"
#include "discord/discord_ipc.hpp"

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include "resource.h"

namespace ReaCord {

extern Discord::Client g_discord_client;

namespace UI {

static void PopulateDialog(HWND hwnd) {
    Config& cfg = Config::Instance();

    // Checkboxes
    SendMessage(GetDlgItem(hwnd, IDC_ENABLE), BM_SETCHECK, cfg.enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hwnd, IDC_INCOGNITO), BM_SETCHECK, cfg.incognito ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hwnd, IDC_CHECK_TRACK_COUNT), BM_SETCHECK, cfg.show_track_count ? BST_CHECKED : BST_UNCHECKED, 0);

    // Combos
    HWND cbProj = GetDlgItem(hwnd, IDC_COMBO_PROJ_NAME);
    SendMessage(cbProj, CB_RESETCONTENT, 0, 0);
    SendMessage(cbProj, CB_ADDSTRING, 0, (LPARAM)"Hidden");
    SendMessage(cbProj, CB_ADDSTRING, 0, (LPARAM)"Name Only (e.g. Track_01)");
    SendMessage(cbProj, CB_ADDSTRING, 0, (LPARAM)"Full Path");
    SendMessage(cbProj, CB_ADDSTRING, 0, (LPARAM)"Generic (\"Working on a Project\")");
    SendMessage(cbProj, CB_SETCURSEL, static_cast<WPARAM>(cfg.project_name_mode), 0);

    HWND cbTime = GetDlgItem(hwnd, IDC_COMBO_SESSION_TIME);
    SendMessage(cbTime, CB_RESETCONTENT, 0, 0);
    SendMessage(cbTime, CB_ADDSTRING, 0, (LPARAM)"Hidden");
    SendMessage(cbTime, CB_ADDSTRING, 0, (LPARAM)"Project Elapsed Time");
    SendMessage(cbTime, CB_ADDSTRING, 0, (LPARAM)"REAPER App Uptime");
    SendMessage(cbTime, CB_SETCURSEL, static_cast<WPARAM>(cfg.session_time_mode), 0);

    HWND cbPlay = GetDlgItem(hwnd, IDC_COMBO_PLAY_STATE);
    SendMessage(cbPlay, CB_RESETCONTENT, 0, 0);
    SendMessage(cbPlay, CB_ADDSTRING, 0, (LPARAM)"Hidden");
    SendMessage(cbPlay, CB_ADDSTRING, 0, (LPARAM)"Simple (Playing, Recording)");
    SendMessage(cbPlay, CB_ADDSTRING, 0, (LPARAM)"Detailed with Tempo (BPM)");
    SendMessage(cbPlay, CB_SETCURSEL, static_cast<WPARAM>(cfg.play_state_mode), 0);

    // Client ID
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CLIENT_ID), cfg.client_id.c_str());

    // Status Text
    std::string discord_status = g_discord_client.GetStatusString();
    std::string status = "Status: " + discord_status;
    if (cfg.client_id == "123456789012345678") {
        status = "Status: Template ID detected";
    } else if (discord_status == "Disconnected") {
        status += " (Discord open?)";
    }
    SetWindowTextA(GetDlgItem(hwnd, IDC_STATUS_TEXT), status.c_str());
}

static void SaveDialog(HWND hwnd) {
    Config& cfg = Config::Instance();

    cfg.enabled = (SendMessage(GetDlgItem(hwnd, IDC_ENABLE), BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.incognito = (SendMessage(GetDlgItem(hwnd, IDC_INCOGNITO), BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.show_track_count = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_TRACK_COUNT), BM_GETCHECK, 0, 0) == BST_CHECKED);

    cfg.project_name_mode = static_cast<ProjectNameMode>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_PROJ_NAME), CB_GETCURSEL, 0, 0));
    cfg.session_time_mode = static_cast<SessionTimeMode>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_SESSION_TIME), CB_GETCURSEL, 0, 0));
    cfg.play_state_mode = static_cast<PlayStateMode>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_PLAY_STATE), CB_GETCURSEL, 0, 0));

    char idBuf[128] = {0};
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CLIENT_ID), idBuf, sizeof(idBuf));
    if (idBuf[0] == '\0') {
        // Fall back to default official ID if user cleared it
        strcpy(idBuf, REACORD_DEFAULT_CLIENT_ID);
        SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CLIENT_ID), idBuf);
    }
    if (cfg.client_id != idBuf) {
        cfg.client_id = idBuf;
        g_discord_client.SetClientId(cfg.client_id);
    }

    cfg.Save();
}

static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            PopulateDialog(hwnd);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    SaveDialog(hwnd);
                    EndDialog(hwnd, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;

                case IDC_APPLY:
                    SaveDialog(hwnd);
                    PopulateDialog(hwnd);
                    return TRUE;

                case IDC_BTN_RESET_DEFAULT:
                    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CLIENT_ID), REACORD_DEFAULT_CLIENT_ID);
                    return TRUE;

                case IDC_BTN_HELP:
                    ShellExecuteA(hwnd, "open", "https://github.com/BartekStaniak/ReaCord/blob/main/docs/DISCORD_APP_SETUP.md", NULL, NULL, SW_SHOWNORMAL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

void ShowSettingsDialog(REACORD_HINSTANCE hInstance, REACORD_HWND parentHwnd) {
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_REACORD_SETTINGS), parentHwnd, DialogProc, 0);
}

} // namespace UI
} // namespace ReaCord

#else // Non-Windows (macOS & Linux)

#include "reaper/reaper_api.h"

namespace ReaCord {

extern Discord::Client g_discord_client;

namespace UI {

void ShowSettingsDialog(REACORD_HINSTANCE hInstance, REACORD_HWND parentHwnd) {
    if (MB) {
        std::string msg = "ReaCord Discord Rich Presence\n\n"
                          "Status: " + g_discord_client.GetStatusString() + "\n"
                          "Active App ID: " + Config::Instance().client_id + "\n\n"
                          "ReaCord is running with the official REAPER Discord application.\n"
                          "To configure privacy settings or view the live Discord card preview, run 'ReaCord_Settings_ImGui.lua' from REAPER's Action List.";
        MB(msg.c_str(), "ReaCord Settings", 0);
    }
}

} // namespace UI
} // namespace ReaCord

#endif
