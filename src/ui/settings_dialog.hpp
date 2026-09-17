#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include "swell/swell.h"
#endif

namespace ReaCord {
namespace UI {

void ShowSettingsDialog(HINSTANCE hInstance, HWND parentHwnd);

} // namespace UI
} // namespace ReaCord
