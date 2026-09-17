#pragma once

#ifdef _WIN32
#include <windows.h>
#define REACORD_HWND HWND
#define REACORD_HINSTANCE HINSTANCE
#else
#define REACORD_HWND void*
#define REACORD_HINSTANCE void*
#endif

namespace ReaCord {
namespace UI {

void ShowSettingsDialog(REACORD_HINSTANCE hInstance, REACORD_HWND parentHwnd);

} // namespace UI
} // namespace ReaCord
