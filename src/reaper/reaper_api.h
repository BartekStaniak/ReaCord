#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define REAPERAPI_MINIMAL

#define REAPERAPI_WANT_plugin_register
#define REAPERAPI_WANT_GetPlayStateEx
#define REAPERAPI_WANT_EnumProjects
#define REAPERAPI_WANT_GetProjectName
#define REAPERAPI_WANT_GetCursorPositionEx
#define REAPERAPI_WANT_CountTracks
#define REAPERAPI_WANT_TimeMap_GetDividedBpmAtTime
#define REAPERAPI_WANT_GetExtState
#define REAPERAPI_WANT_SetExtState
#define REAPERAPI_WANT_HasExtState
#define REAPERAPI_WANT_time_precise
#define REAPERAPI_WANT_GetMainHwnd
#define REAPERAPI_WANT_ShowConsoleMsg
#define REAPERAPI_WANT_MB
#define REAPERAPI_WANT_NamedCommandLookup
#define REAPERAPI_WANT_Main_OnCommand

#include "reaper_plugin.h"
#include "reaper_plugin_functions.h"
