# ReaCord

![ReaCord Banner](assets/discord/banner.jpg)

[![Latest Release](https://img.shields.io/github/v/release/BartekStaniak/ReaCord?color=blue&label=release)](https://github.com/BartekStaniak/ReaCord/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/BartekStaniak/ReaCord/ci.yml?branch=main&label=build)](https://github.com/BartekStaniak/ReaCord/actions/workflows/ci.yml)
![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey)
[![ReaPack Compatible](https://img.shields.io/badge/ReaPack-compatible-brightgreen)](https://raw.githubusercontent.com/BartekStaniak/ReaCord/main/index.xml)
[![License: MIT](https://img.shields.io/github/license/BartekStaniak/ReaCord?color=green)](LICENSE)

> Lightweight Discord Rich Presence for Cockos REAPER. No external helper scripts, no background Node/Python processes, and zero audio thread impact.

---

## Overview

Most Discord Rich Presence setups for REAPER rely on external Python scripts, Node.js bridges, or background utilities that sit in your system tray. ReaCord doesn't. It's a native C++ plugin (`reaper_reacord`) that runs directly inside REAPER and talks straight to Discord over local IPC.

Because audio performance comes first, all presence updates run on a detached worker thread. The real-time audio engine is never touched, RAM usage stays under 2 MB, and you don't have to keep another console window open while working on a track.

---

## Features

- **Self-contained**: Native extension (`.dll` on Windows, `.dylib` on macOS, `.so` on Linux). No secondary helper apps, terminal windows, or interpreters to install.
- **Audio-safe**: Presence updates run asynchronously on a low-priority background thread at ~0.6 Hz. The DSP/audio thread is never blocked.
- **Low footprint**: Uses under 2 MB of memory and less than 0.01% CPU. Updates are hashed so packets are only sent when project state actually changes.
- **Privacy controls**: Customize what your Discord profile shows:
  - Project name: full filename, project name only, generic placeholder (*"Working on a Project"*), or hidden entirely.
  - Session timer: project playback time, total REAPER uptime, or off.
  - Playback status: show playback with tempo (*"Playing @ 128 BPM"*), simple state (*"Recording"*), or hidden.
  - Track count: toggle on or off.
  - One-click Incognito: an instant stealth action that hides all project details and track counts.
- **Two configuration UIs**: A native REAPER settings window (built with SWELL) and an optional ReaImGui script with a live Discord preview card.
- **ReaPack support**: Install once and get automatic updates directly through REAPER's package manager.

---

## Installation

### Method 1: Via ReaPack (Recommended)

1. Open REAPER.
2. Go to **Extensions > ReaPack > Import Repositories...**
3. Paste the ReaCord repository URL:
   ```text
   https://raw.githubusercontent.com/BartekStaniak/ReaCord/main/index.xml
   ```
4. Open **Extensions > ReaPack > Browse Packages...**
5. Search for `ReaCord`, right-click and select **Install**.
6. Click **Apply** in the bottom-right corner and restart REAPER.

---

### Method 2: Manual Installation

1. Download the pre-compiled binary for your operating system from [Releases](https://github.com/BartekStaniak/ReaCord/releases):
   - **Windows (x64)**: `reaper_reacord64.dll`
   - **macOS (Universal - Apple Silicon & Intel)**: `reaper_reacord.dylib`
   - **Linux (x86_64)**: `reaper_reacord-x86_64.so`
2. In REAPER, go to **Options > Show REAPER resource path in explorer/finder**.
3. Open the `UserPlugins` folder (create it if it doesn't exist).
4. Drop the downloaded binary into `UserPlugins`.
5. Restart REAPER.

---

## Configuration

ReaCord provides multiple ways to configure your presence:

### 1. Extensions Menu
Open REAPER's top menu bar and select:  
**Extensions > ReaCord Settings...**

### 2. Action List & Shortcuts
Press `?` to open REAPER's **Action List**, search for `ReaCord: Open Settings...`, and click **Run**. You can bind this action to any shortcut key or toolbar button.

ReaCord also provides a toggle action for stealth mode:
- `ReaCord: Toggle Incognito Mode`

### 3. ReaImGui Companion Script (Optional)
If you have **ReaImGui** installed, you can launch the companion script from the Action List:
```text
scripts/ReaCord_Settings_ImGui.lua
```
This opens a floating window with live socket connection telemetry and a real-time preview of how your card looks in Discord.

---

## Discord Application & Artwork

ReaCord works immediately using the default pre-configured REAPER Application ID (`1462972195658534965`).

If you prefer to register your own custom Discord app or upload custom art, pre-rendered 512x512 transparent PNG assets are provided in [`assets/discord/`](assets/discord/):
- `reacord_logo.png` (hybrid REAPER / Discord diagonal split emblem)
- `reaper_logo.png` (classic REAPER guitar pick)
- `play.png` (playback badge)
- `record.png` (recording badge)
- `pause.png` (pause badge)
- `stop.png` (idle badge)

For full setup instructions, see [docs/DISCORD_APP_SETUP.md](docs/DISCORD_APP_SETUP.md).

---

## Architecture

```
[ REAPER Realtime Audio Engine ] ---> [ UNTOUCHED / ZERO OVERHEAD ]
               |
[ REAPER Main Thread ] (Timer hook @ 0.6 Hz)
               |  (Sanitizes state & applies privacy options)
        [ Lock-Free Snapshot ]
               |  (Double-buffered exchange)
[ ReaCord Worker Thread ] (Asynchronous Event Loop)
               |  (Non-blocking rate-limiting & dirty hash check)
[ Local IPC Pipe / Domain Socket ]
               |
[ Discord Desktop Client ]
```

---

## Building From Source

### Prerequisites
- CMake 3.16+
- C++17 compiler:
  - **Windows**: Visual Studio 2019/2022 (MSVC)
  - **macOS**: Xcode / Apple Clang
  - **Linux**: GCC or Clang + GTK3 headers (`sudo apt install libgtk-3-dev`)

### Build Steps

```bash
# 1. Clone repository with submodules
git clone --recursive https://github.com/BartekStaniak/ReaCord.git
cd ReaCord

# 2. Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile
cmake --build build --config Release
```

Compiled binaries are output to `build/Release/` (Windows) or `build/` (macOS/Linux).

---

## Author

Developed by **Bartek Staniak**  
GitHub: [@BartekStaniak](https://github.com/BartekStaniak)

---

## License & Policies

- **License:** Licensed under the [MIT License](LICENSE) by Bartek Staniak. Cockos WDL and REAPER SDK are licensed under their respective Cockos licenses.
- **Terms of Service:** [TERMS_OF_SERVICE.md](TERMS_OF_SERVICE.md)
- **Privacy Policy:** [PRIVACY_POLICY.md](PRIVACY_POLICY.md)
