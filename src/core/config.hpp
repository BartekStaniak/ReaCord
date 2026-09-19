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

enum class IconStyle {
    ReaperClassic = 0,
    ReaCordHybrid = 1
};

struct Config {
    bool enabled = true;
    bool incognito = false;
    ProjectNameMode project_name_mode = ProjectNameMode::NameOnly;
    SessionTimeMode session_time_mode = SessionTimeMode::ProjectElapsed;
    PlayStateMode play_state_mode = PlayStateMode::DetailedBpm;
    IconStyle icon_style = IconStyle::ReaperClassic;
    bool show_track_count = true;
#define REACORD_DEFAULT_CLIENT_ID "1462972195658534965"

    std::string client_id = REACORD_DEFAULT_CLIENT_ID; // Official ReaCord App ID
    std::string large_image_key = "reaper_logo";
    int idle_timeout_mins = 15;

    inline std::string GetEffectiveLargeImageKey() const {
        if (!large_image_key.empty() && large_image_key != "reaper_logo" && large_image_key != "reacord_logo") {
            return large_image_key;
        }
        return (icon_style == IconStyle::ReaCordHybrid) ? "reacord_logo" : "reaper_logo";
    }

    // Serialization to / from REAPER ExtState (persisted in reaper.ini)
    void Load();
    void Save() const;

    // Singleton access
    static Config& Instance();
};

} // namespace ReaCord
