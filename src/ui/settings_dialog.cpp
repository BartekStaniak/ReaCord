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
    SendMessage(cbTime, CB_ADDSTRING, 0, (LPARAM)"Project Active Time (ExtState)");
    SendMessage(cbTime, CB_SETCURSEL, static_cast<WPARAM>(cfg.session_time_mode), 0);

    HWND cbPlay = GetDlgItem(hwnd, IDC_COMBO_PLAY_STATE);
    SendMessage(cbPlay, CB_RESETCONTENT, 0, 0);
    SendMessage(cbPlay, CB_ADDSTRING, 0, (LPARAM)"Hidden");
    SendMessage(cbPlay, CB_ADDSTRING, 0, (LPARAM)"Simple (Playing, Recording)");
    SendMessage(cbPlay, CB_ADDSTRING, 0, (LPARAM)"Detailed with Tempo (BPM)");
    SendMessage(cbPlay, CB_SETCURSEL, static_cast<WPARAM>(cfg.play_state_mode), 0);

    HWND cbIcon = GetDlgItem(hwnd, IDC_COMBO_ICON_STYLE);
    SendMessage(cbIcon, CB_RESETCONTENT, 0, 0);
    SendMessage(cbIcon, CB_ADDSTRING, 0, (LPARAM)"REAPER Logo (Classic)");
    SendMessage(cbIcon, CB_ADDSTRING, 0, (LPARAM)"ReaCord Emblem (Hybrid)");
    SendMessage(cbIcon, CB_SETCURSEL, static_cast<WPARAM>(cfg.icon_style), 0);

    SendMessage(GetDlgItem(hwnd, IDC_CHECK_EXTSTATE_TEXT), BM_SETCHECK, cfg.extstate_in_state_text ? BST_CHECKED : BST_UNCHECKED, 0);
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_EXTSTATE_SECTION), cfg.extstate_section.c_str());
    SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_EXTSTATE_KEY), cfg.extstate_key.c_str());

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
    cfg.extstate_in_state_text = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_EXTSTATE_TEXT), BM_GETCHECK, 0, 0) == BST_CHECKED);

    cfg.project_name_mode = static_cast<ProjectNameMode>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_PROJ_NAME), CB_GETCURSEL, 0, 0));
    cfg.session_time_mode = static_cast<SessionTimeMode>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_SESSION_TIME), CB_GETCURSEL, 0, 0));
    cfg.play_state_mode = static_cast<PlayStateMode>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_PLAY_STATE), CB_GETCURSEL, 0, 0));
    cfg.icon_style = static_cast<IconStyle>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_ICON_STYLE), CB_GETCURSEL, 0, 0));
    cfg.large_image_key = (cfg.icon_style == IconStyle::ReaCordHybrid) ? "reacord_logo" : "reaper_logo";

    char secBuf[128] = {0};
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_EXTSTATE_SECTION), secBuf, sizeof(secBuf));
    if (secBuf[0]) cfg.extstate_section = secBuf;

    char keyBuf[128] = {0};
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_EXTSTATE_KEY), keyBuf, sizeof(keyBuf));
    if (keyBuf[0]) cfg.extstate_key = keyBuf;

    char idBuf[128] = {0};
    GetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CLIENT_ID), idBuf, sizeof(idBuf));
    
    // Sanitize to digits only
    std::string sanitized_id;
    for (int i = 0; idBuf[i] != '\0'; ++i) {
        if (isdigit(static_cast<unsigned char>(idBuf[i]))) {
            sanitized_id += idBuf[i];
        }
    }

    if (sanitized_id.empty() || sanitized_id == "123456789012345678") {
        sanitized_id = REACORD_DEFAULT_CLIENT_ID;
        SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT_CLIENT_ID), sanitized_id.c_str());
    }

    if (cfg.client_id != sanitized_id) {
        cfg.client_id = sanitized_id;
        g_discord_client.SetClientId(cfg.client_id);
    }

    cfg.Save();
}

static bool s_advanced_expanded = true;
static int s_delta_y = 54;

static void SetAdvancedExpanded(HWND hwnd, bool expand) {
    if (s_advanced_expanded == expand) return;
    s_advanced_expanded = expand;

    int showCmd = expand ? SW_SHOW : SW_HIDE;
    ShowWindow(GetDlgItem(hwnd, IDC_GROUP_ADVANCED), showCmd);
    ShowWindow(GetDlgItem(hwnd, IDC_STATIC_EXTSTATE), showCmd);
    ShowWindow(GetDlgItem(hwnd, IDC_EDIT_EXTSTATE_SECTION), showCmd);
    ShowWindow(GetDlgItem(hwnd, IDC_EDIT_EXTSTATE_KEY), showCmd);
    ShowWindow(GetDlgItem(hwnd, IDC_CHECK_EXTSTATE_TEXT), showCmd);

    int shift_y = expand ? s_delta_y : -s_delta_y;

    const int bottom_controls[] = {
        IDC_BTN_HELP,
        IDC_STATUS_TEXT,
        IDOK,
        IDCANCEL,
        IDC_APPLY
    };

    for (int id : bottom_controls) {
        HWND hCtrl = GetDlgItem(hwnd, id);
        if (hCtrl) {
            RECT rc;
            GetWindowRect(hCtrl, &rc);
            POINT pt = { rc.left, rc.top };
            ScreenToClient(hwnd, &pt);
            SetWindowPos(hCtrl, NULL, pt.x, pt.y + shift_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    RECT rcDlg;
    GetWindowRect(hwnd, &rcDlg);
    SetWindowPos(hwnd, NULL, 0, 0, rcDlg.right - rcDlg.left, (rcDlg.bottom - rcDlg.top) + shift_y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    SendMessage(GetDlgItem(hwnd, IDC_CHECK_SHOW_ADVANCED), BM_SETCHECK, expand ? BST_CHECKED : BST_UNCHECKED, 0);
}

static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            PopulateDialog(hwnd);

            HWND hGrp = GetDlgItem(hwnd, IDC_GROUP_ADVANCED);
            if (hGrp) {
                RECT rcGrp;
                GetWindowRect(hGrp, &rcGrp);
                s_delta_y = (rcGrp.bottom - rcGrp.top) + 8;
                if (s_delta_y <= 0) s_delta_y = 54;
            }

            Config& cfg = Config::Instance();
            bool should_expand = (cfg.session_time_mode == SessionTimeMode::ProjectExtState || cfg.extstate_in_state_text);
            s_advanced_expanded = true;
            if (!should_expand) {
                SetAdvancedExpanded(hwnd, false);
            } else {
                SendMessage(GetDlgItem(hwnd, IDC_CHECK_SHOW_ADVANCED), BM_SETCHECK, BST_CHECKED, 0);
            }
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHECK_SHOW_ADVANCED: {
                    bool is_checked = (SendMessage(GetDlgItem(hwnd, IDC_CHECK_SHOW_ADVANCED), BM_GETCHECK, 0, 0) == BST_CHECKED);
                    SetAdvancedExpanded(hwnd, is_checked);
                    return TRUE;
                }

                case IDC_COMBO_SESSION_TIME: {
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int sel = static_cast<int>(SendMessage(GetDlgItem(hwnd, IDC_COMBO_SESSION_TIME), CB_GETCURSEL, 0, 0));
                        if (sel == static_cast<int>(SessionTimeMode::ProjectExtState)) {
                            if (!s_advanced_expanded) {
                                SetAdvancedExpanded(hwnd, true);
                            }
                        }
                    }
                    return TRUE;
                }

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
