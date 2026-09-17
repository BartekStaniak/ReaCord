#pragma once

#include <string>

namespace ReaCord {

enum class ProjectNameMode {
    Hidden = 0,
    NameOnly = 1,
    FullPath = 2,
    Generic = 3
};

enum class SessionTimeMode {
    Hidden = 0,
    ProjectElapsed = 1,
    DawUptime = 2
};

enum class PlayStateMode {
    Hidden = 0,
    Simple = 1,
    DetailedBpm = 2
};

struct Config {
    bool enabled = true;
    bool incognito = false;
    ProjectNameMode project_name_mode = ProjectNameMode::NameOnly;
    SessionTimeMode session_time_mode = SessionTimeMode::ProjectElapsed;
    PlayStateMode play_state_mode = PlayStateMode::DetailedBpm;
    bool show_track_count = true;
    std::string client_id = "123456789012345678"; // Template default
    std::string large_image_key = "reaper_logo";
    int idle_timeout_mins = 15;

    // Serialization to / from REAPER ExtState (persisted in reaper.ini)
    void Load();
    void Save() const;

    // Singleton access
    static Config& Instance();
};

} // namespace ReaCord
