#include "reaper_observer.hpp"
#include "reaper/reaper_api.h"
#include "core/state_snapshot.hpp"
#include <ctime>
#include <cmath>
#include <sstream>

namespace ReaCord {

void Observer::Initialize(Discord::Client* client) {
    discord_client_ = client;
    app_start_time_ = static_cast<int64_t>(std::time(nullptr));
    project_start_time_ = app_start_time_;
    last_poll_time_ = 0.0;
    last_project_name_.clear();
}

void Observer::Shutdown() {
    if (discord_client_) {
        discord_client_->ClearActivity();
    }
    discord_client_ = nullptr;
}

static std::string StripPathAndExt(const std::string& full_path) {
    if (full_path.empty()) return "Unsaved Project";

    size_t last_slash = full_path.find_last_of("/\\");
    std::string filename = (last_slash == std::string::npos) ? full_path : full_path.substr(last_slash + 1);

    size_t last_dot = filename.find_last_of('.');
    if (last_dot != std::string::npos && last_dot > 0) {
        filename = filename.substr(0, last_dot);
    }
    return filename;
}

void Observer::OnTimerTick() {
    if (!discord_client_) return;

    double now = time_precise ? time_precise() : 0.0;
    // Throttle polling to at most once every 1.5 seconds to ensure zero CPU impact
    if (now - last_poll_time_ < 1.5) {
        return;
    }
    last_poll_time_ = now;

    const Config& cfg = Config::Instance();

    // 1. Check if disabled
    if (!cfg.enabled) {
        discord_client_->ClearActivity();
        return;
    }

    Discord::Activity act;
    act.is_active = true;
    act.large_image = cfg.GetEffectiveLargeImageKey();
    act.large_text = "Cockos REAPER";

    // 2. Incognito Mode: simple stealth display
    if (cfg.incognito) {
        act.details = "Working in REAPER";
        act.state = "Incognito";
        StateStore::Instance().SetActivity(act);
        discord_client_->UpdateActivity(act);
        return;
    }

    // 3. Project Name & Timing
    char proj_fn[1024] = {0};
    ReaProject* current_proj = EnumProjects ? EnumProjects(-1, proj_fn, sizeof(proj_fn)) : nullptr;
    std::string raw_proj_path = proj_fn;

    // Detect project change to reset project timer
    if (raw_proj_path != last_project_name_) {
        last_project_name_ = raw_proj_path;
        project_start_time_ = static_cast<int64_t>(std::time(nullptr));
    }

    switch (cfg.project_name_mode) {
        case ProjectNameMode::NameOnly:
            act.details = "Project: " + StripPathAndExt(raw_proj_path);
            break;
        case ProjectNameMode::FullPath:
            act.details = raw_proj_path.empty() ? "Unsaved Project" : raw_proj_path;
            break;
        case ProjectNameMode::Generic:
            act.details = "Working on a Project";
            break;
        case ProjectNameMode::Hidden:
        default:
            break;
    }

    // Session time
    switch (cfg.session_time_mode) {
        case SessionTimeMode::ProjectElapsed:
            act.start_time = project_start_time_;
            break;
        case SessionTimeMode::DawUptime:
            act.start_time = app_start_time_;
            break;
        case SessionTimeMode::Hidden:
        default:
            act.start_time = 0;
            break;
    }

    // 4. Playback / Record State
    int playstate = GetPlayStateEx ? GetPlayStateEx(current_proj) : 0;
    // playstate: 0 = stopped, 1 = playing, 2 = paused, 4 = recording, 5 = record+play
    bool is_playing = (playstate & 1) != 0;
    bool is_paused = (playstate & 2) != 0;
    bool is_recording = (playstate & 4) != 0;

    double curpos = GetCursorPositionEx ? GetCursorPositionEx(current_proj) : 0.0;
    int bpm = 120;
    if (TimeMap_GetDividedBpmAtTime) {
        bpm = static_cast<int>(std::round(TimeMap_GetDividedBpmAtTime(curpos)));
    }

    std::string state_str;
    std::string small_icon;
    std::string small_desc;

    if (is_recording) {
        small_icon = "record";
        small_desc = "Recording";
        state_str = (cfg.play_state_mode == PlayStateMode::DetailedBpm) 
            ? "Recording (" + std::to_string(bpm) + " BPM)" 
            : "Recording";
    } else if (is_playing) {
        small_icon = "play";
        small_desc = "Playing";
        state_str = (cfg.play_state_mode == PlayStateMode::DetailedBpm) 
            ? "Playing (" + std::to_string(bpm) + " BPM)" 
            : "Playing";
    } else if (is_paused) {
        small_icon = "pause";
        small_desc = "Paused";
        state_str = "Paused";
    } else {
        small_icon = "stop";
        small_desc = "Editing";
        state_str = "Editing";
    }

    if (cfg.play_state_mode == PlayStateMode::Hidden) {
        act.state.clear();
        act.small_image.clear();
        act.small_text.clear();
    } else {
        act.small_image = small_icon;
        act.small_text = small_desc;

        // Track count append
        if (cfg.show_track_count && CountTracks) {
            int track_count = CountTracks(current_proj);
            state_str += " • " + std::to_string(track_count) + (track_count == 1 ? " Track" : " Tracks");
        }
        act.state = state_str;
    }

    // Save snapshot and push to Discord client
    StateStore::Instance().SetActivity(act);
    discord_client_->UpdateActivity(act);
}

} // namespace ReaCord
