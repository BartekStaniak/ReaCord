-- @description ReaCord Settings (ReaImGui Modern Interface)
-- @author Bartek Staniak
-- @version 1.0.3-beta6
-- @about
--   Modern hardware-accelerated GUI for ReaCord with live Discord profile card preview.
--   Provides real-time configuration of privacy opt-ins and presence attributes.

local ctx

-- Verify ReaImGui availability
if not reaper.ImGui_CreateContext then
    local res = reaper.MB("ReaImGui is required to run the modern settings interface.\nWould you like to open the native settings dialog instead?", "ReaCord", 4)
    if res == 6 then
        -- Trigger native settings action ID
        local cmd = reaper.NamedCommandLookup("_REACORD_OPEN_SETTINGS")
        if cmd > 0 then reaper.Main_OnCommand(cmd, 0) end
    end
    return
end

ctx = reaper.ImGui_CreateContext('ReaCord Settings')

-- Helper functions to read/write ReaCord config via API or ExtState
local function GetConfig(key, default_val)
    if reaper.ReaCord_GetConfig then
        local val = reaper.ReaCord_GetConfig(key)
        if val and val ~= "" then return val end
    end
    local val = reaper.GetExtState("ReaCord", key)
    if val and val ~= "" then return val end
    return default_val
end

local function SetConfig(key, val)
    if reaper.ReaCord_SetConfig then
        reaper.ReaCord_SetConfig(key, tostring(val))
    end
    reaper.SetExtState("ReaCord", key, tostring(val), true)
end

-- State variables
local enabled = GetConfig("enabled", "1") == "1"
local incognito = GetConfig("incognito", "0") == "1"
local proj_mode = tonumber(GetConfig("project_name_mode", "1")) or 1
local time_mode = tonumber(GetConfig("session_time_mode", "1")) or 1
local play_mode = tonumber(GetConfig("play_state_mode", "2")) or 2
local icon_style = tonumber(GetConfig("icon_style", "0")) or 0
local large_key = GetConfig("large_image_key", "")
if large_key == "reacord_logo" then icon_style = 1
elseif large_key == "reaper_logo" then icon_style = 0 end

local DEFAULT_CLIENT_ID = "1462972195658534965"
local client_id = GetConfig("client_id", DEFAULT_CLIENT_ID)
if client_id == "" or client_id == "123456789012345678" then 
    client_id = DEFAULT_CLIENT_ID 
    SetConfig("client_id", client_id)
end

local extstate_sec = GetConfig("extstate_section", "PROJECT_TIME")
local extstate_key = GetConfig("extstate_key", "active_time")
local extstate_text = GetConfig("extstate_in_state_text", "0") == "1"
local prefer_reaimgui = GetConfig("prefer_reaimgui", "0") == "1"

local proj_options = { "Hidden", "Project Name Only", "Full Path", "Generic (\"Working on a Project\")" }
local time_options = { "Hidden", "Project Elapsed Time", "REAPER Uptime", "Project Active Time (ExtState)" }
local play_options = { "Hidden", "Simple (Playing, Recording)", "Detailed with Tempo (BPM)" }
local icon_options = { "REAPER Logo (Classic)", "ReaCord Emblem (Hybrid)" }

local function QueryLiveExtState()
    if not reaper.GetProjExtState then return nil end
    local ok, val = reaper.GetProjExtState(0, extstate_sec, extstate_key)
    if ok == 1 and val ~= "" then
        return val
    end
    return nil
end

local function RenderDiscordPreview()
    reaper.ImGui_SeparatorText(ctx, "Live Discord Profile Preview")

    -- Card background box
    reaper.ImGui_PushStyleColor(ctx, reaper.ImGui_Col_ChildBg(), 0x1E1F22FF)
    reaper.ImGui_PushStyleVar(ctx, reaper.ImGui_StyleVar_ChildRounding(), 8.0)

    if reaper.ImGui_BeginChild(ctx, "DiscordCard", 0, 110, reaper.ImGui_ChildFlags_Borders()) then
        reaper.ImGui_Dummy(ctx, 4, 4)
        reaper.ImGui_SameLine(ctx)

        -- Large icon placeholder
        reaper.ImGui_PushStyleColor(ctx, reaper.ImGui_Col_Button(), 0x2B2D31FF)
        local logo_label = (icon_style == 1) and "ReaCord\nHybrid" or "REAPER\nClassic"
        reaper.ImGui_Button(ctx, logo_label, 64, 64)
        reaper.ImGui_PopStyleColor(ctx)

        reaper.ImGui_SameLine(ctx, 0, 16)

        -- Text presence lines
        reaper.ImGui_BeginGroup(ctx)
        reaper.ImGui_TextColored(ctx, 0xFFFFFFFF, "Cockos REAPER")
        
        if not enabled then
            reaper.ImGui_TextColored(ctx, 0x80848EFF, "[Discord Rich Presence Disabled]")
        elseif incognito then
            reaper.ImGui_TextColored(ctx, 0xDBDEE1FF, "Working in REAPER")
            reaper.ImGui_TextColored(ctx, 0x949BA4FF, "Incognito")
        else
            -- Details line
            local details_str = "Working on a Project"
            if proj_mode == 0 then details_str = ""
            elseif proj_mode == 1 then details_str = "Project: LeadVocal_Mix"
            elseif proj_mode == 2 then details_str = "F:/Projects/Album/LeadVocal_Mix.rpp" end
            if details_str ~= "" then
                reaper.ImGui_TextColored(ctx, 0xDBDEE1FF, details_str)
            end

            -- State line
            local state_str = "Editing"
            if play_mode == 1 then state_str = "Playing"
            elseif play_mode == 2 then state_str = "Playing (128 BPM)"
            elseif play_mode == 0 then state_str = "" end

            if track_count and state_str ~= "" then
                state_str = state_str .. " • 24 Tracks"
            end
            if extstate_text and state_str ~= "" then
                state_str = state_str .. " • 4h 12m"
            end
            if state_str ~= "" then
                reaper.ImGui_TextColored(ctx, 0x949BA4FF, state_str)
            end

            -- Timestamp line
            if time_mode == 3 then
                reaper.ImGui_TextColored(ctx, 0x949BA4FF, "04:12:35 elapsed (Active ExtState)")
            elseif time_mode ~= 0 then
                reaper.ImGui_TextColored(ctx, 0x949BA4FF, "01:24:50 elapsed")
            end
        end

        reaper.ImGui_EndGroup(ctx)
        reaper.ImGui_EndChild(ctx)
    end

    reaper.ImGui_PopStyleVar(ctx)
    reaper.ImGui_PopStyleColor(ctx)
end

local function Loop()
    local visible, open = reaper.ImGui_Begin(ctx, 'ReaCord Preferences', true, reaper.ImGui_WindowFlags_AlwaysAutoResize())
    if visible then
        -- Connection Status header
        local status_str = reaper.ReaCord_GetStatus and reaper.ReaCord_GetStatus() or "Unknown"
        local status_col = 0x949BA4FF
        if status_str == "Connected" then 
            status_col = 0x57F287FF
        elseif status_str == "Connecting..." then 
            status_col = 0xFEE75CFF
        elseif status_str == "Disconnected" then 
            status_col = 0xED4245FF 
            status_str = status_str .. " (Check if Discord app is open)"
        end

        reaper.ImGui_Text(ctx, "Discord IPC Status: ")
        reaper.ImGui_SameLine(ctx)
        reaper.ImGui_TextColored(ctx, status_col, status_str)
        reaper.ImGui_Spacing(ctx)

        -- Master toggles
        local changed
        changed, enabled = reaper.ImGui_Checkbox(ctx, 'Enable Discord Rich Presence', enabled)
        if changed then SetConfig("enabled", enabled and "1" or "0") end

        changed, incognito = reaper.ImGui_Checkbox(ctx, 'Incognito Mode (Privacy Stealth)', incognito)
        if changed then SetConfig("incognito", incognito and "1" or "0") end

        reaper.ImGui_SeparatorText(ctx, "Privacy & Data Opt-Ins")

        -- Project Name Combo
        if reaper.ImGui_BeginCombo(ctx, "Project Name", proj_options[proj_mode + 1]) then
            for i, opt in ipairs(proj_options) do
                local is_selected = (proj_mode == (i - 1))
                if reaper.ImGui_Selectable(ctx, opt, is_selected) then
                    proj_mode = i - 1
                    SetConfig("project_name_mode", proj_mode)
                end
            end
            reaper.ImGui_EndCombo(ctx)
        end

        -- Session Time Combo
        if reaper.ImGui_BeginCombo(ctx, "Session Time", time_options[time_mode + 1]) then
            for i, opt in ipairs(time_options) do
                local is_selected = (time_mode == (i - 1))
                if reaper.ImGui_Selectable(ctx, opt, is_selected) then
                    time_mode = i - 1
                    SetConfig("session_time_mode", time_mode)
                end
            end
            reaper.ImGui_EndCombo(ctx)
        end

        -- Play State Combo
        if reaper.ImGui_BeginCombo(ctx, "Play / Record State", play_options[play_mode + 1]) then
            for i, opt in ipairs(play_options) do
                local is_selected = (play_mode == (i - 1))
                if reaper.ImGui_Selectable(ctx, opt, is_selected) then
                    play_mode = i - 1
                    SetConfig("play_state_mode", play_mode)
                end
            end
            reaper.ImGui_EndCombo(ctx)
        end

        -- Profile Card Logo Combo
        if reaper.ImGui_BeginCombo(ctx, "Profile Card Logo", icon_options[icon_style + 1]) then
            for i, opt in ipairs(icon_options) do
                local is_selected = (icon_style == (i - 1))
                if reaper.ImGui_Selectable(ctx, opt, is_selected) then
                    icon_style = i - 1
                    SetConfig("icon_style", icon_style)
                    SetConfig("large_image_key", (icon_style == 1) and "reacord_logo" or "reaper_logo")
                end
            end
            reaper.ImGui_EndCombo(ctx)
        end

        changed, track_count = reaper.ImGui_Checkbox(ctx, 'Show Track Count', track_count)
        if changed then SetConfig("show_track_count", track_count and "1" or "0") end

        -- Advanced Settings Collapsible Header
        reaper.ImGui_Spacing(ctx)
        if time_mode == 3 or client_id ~= DEFAULT_CLIENT_ID then
            reaper.ImGui_SetNextItemOpen(ctx, true, reaper.ImGui_Cond_Appearing())
        end
        if reaper.ImGui_CollapsingHeader(ctx, "Advanced Settings (Client ID & Custom Timers)") then
            reaper.ImGui_PushStyleColor(ctx, reaper.ImGui_Col_ChildBg(), 0x232428FF)
            if reaper.ImGui_BeginChild(ctx, "AdvancedSettingsBox", 0, 195, reaper.ImGui_ChildFlags_Borders()) then
                -- Discord Client ID
                reaper.ImGui_TextColored(ctx, 0x5865F2FF, "Custom Discord Client ID:")
                changed, client_id = reaper.ImGui_InputText(ctx, "Client ID", client_id)
                if changed then SetConfig("client_id", client_id) end

                reaper.ImGui_SameLine(ctx)
                if reaper.ImGui_Button(ctx, "Default ID") then
                    client_id = DEFAULT_CLIENT_ID
                    SetConfig("client_id", client_id)
                end

                if client_id == "123456789012345678" then
                    reaper.ImGui_TextColored(ctx, 0xFEE75CFF, "Warning: Template Client ID detected! Click 'Default ID'.")
                else
                    reaper.ImGui_TextColored(ctx, 0x80848EFF, "Default: Official REAPER App (" .. DEFAULT_CLIENT_ID .. ")")
                end

                reaper.ImGui_Separator(ctx)

                -- ExtState Custom Project Timer
                reaper.ImGui_TextColored(ctx, 0x5865F2FF, "Project ExtState Active Timer:")
                changed, extstate_text = reaper.ImGui_Checkbox(ctx, 'Append active time to playback state text', extstate_text)
                if changed then SetConfig("extstate_in_state_text", extstate_text and "1" or "0") end

                changed, extstate_sec = reaper.ImGui_InputText(ctx, "Section", extstate_sec)
                if changed then SetConfig("extstate_section", extstate_sec) end

                changed, extstate_key = reaper.ImGui_InputText(ctx, "Key", extstate_key)
                if changed then SetConfig("extstate_key", extstate_key) end

                local live_val = QueryLiveExtState()
                if live_val then
                    reaper.ImGui_TextColored(ctx, 0x57F287FF, "Current Project Value: \"" .. live_val .. "\"")
                else
                    reaper.ImGui_TextColored(ctx, 0x949BA4FF, "Current Project Value: [Not found in current project]")
                end
                reaper.ImGui_EndChild(ctx)
            end
            reaper.ImGui_PopStyleColor(ctx)
        end

        reaper.ImGui_Spacing(ctx)
        RenderDiscordPreview()

        reaper.ImGui_Spacing(ctx)
        local is_windows = reaper.GetOS():match("Win") ~= nil
        if is_windows then
            changed, prefer_reaimgui = reaper.ImGui_Checkbox(ctx, "Use Modern ReaImGui by default for Extensions menu", prefer_reaimgui)
            if changed then
                SetConfig("prefer_reaimgui", prefer_reaimgui and "1" or "0")
            end

            reaper.ImGui_Spacing(ctx)
            if reaper.ImGui_Button(ctx, "Switch to Classic Win32 Dialog", 220, 0) then
                local cmd = reaper.NamedCommandLookup("_REACORD_OPEN_SETTINGS_NATIVE")
                if cmd <= 0 then
                    cmd = reaper.NamedCommandLookup("_REACORD_OPEN_SETTINGS")
                end
                open = false
                if cmd > 0 then
                    reaper.defer(function()
                        reaper.Main_OnCommand(cmd, 0)
                    end)
                end
            end
            reaper.ImGui_SameLine(ctx)
        end

        if reaper.ImGui_Button(ctx, "Close", 120, 0) then
            open = false
        end

        reaper.ImGui_End(ctx)
    end

    if open then
        reaper.defer(Loop)
    else
        reaper.ImGui_DestroyContext(ctx)
    end
end

reaper.defer(Loop)
