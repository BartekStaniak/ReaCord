#pragma once

#include "reaper/reaper_api.h"
#include "reaper_observer.hpp"

namespace ReaCord {

class ReaCordCsurf : public IReaperControlSurface {
public:
    ReaCordCsurf() = default;
    virtual ~ReaCordCsurf() = default;

    const char* GetTypeString() override {
        return "REACORD";
    }

    const char* GetDescString() override {
        return "ReaCord Discord Presence Controller";
    }

    const char* GetConfigString() override {
        return "";
    }

    void CloseNoReset() override {}
    void Run() override {}

    // Instant transport change callback (Play, Pause, Stop, Record)
    void SetPlayState(bool /*play*/, bool /*pause*/, bool /*rec*/) override {
        Observer::Instance().TriggerInstantUpdate();
    }

    // Instant track structure change callback (Add, Delete, Reorder)
    void SetTrackListChange() override {
        Observer::Instance().TriggerInstantUpdate();
    }

    // Extended callbacks (e.g. tempo/BPM changes)
    int Extended(int call, void* /*parm1*/, void* /*parm2*/, void* /*parm3*/) override {
        if (call == CSURF_EXT_SETBPMANDPLAYRATE) {
            Observer::Instance().TriggerInstantUpdate();
            return 1;
        }
        return 0;
    }
};

extern ReaCordCsurf g_csurf_instance;

} // namespace ReaCord
