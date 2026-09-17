# Privacy Policy for ReaCord

**Last Updated:** September 17, 2026  
**Developer:** Bartek Staniak ("Developer", "we", "us", or "our")  
**Project:** ReaCord (Discord Rich Presence for Cockos REAPER)  
**Repository:** [https://github.com/BartekStaniak/ReaCord](https://github.com/BartekStaniak/ReaCord)

---

## 1. Introduction

ReaCord is an open-source native C++ extension for Cockos REAPER that displays your active REAPER digital audio workstation session status on your Discord profile via Discord's Rich Presence API.

We take your privacy seriously. This Privacy Policy explains what information ReaCord processes, how that information is handled, and your choices and controls regarding your data.

**Key Principle:** ReaCord is designed from the ground up to be **local-only, privacy-respecting, and completely free of external telemetry**. We do not operate remote servers, do not collect personal analytics, and do not track or store your personal information.

---

## 2. Information Processed by ReaCord

To display your REAPER activity on Discord, ReaCord reads the following session information from your active REAPER workspace in real time:

- **Project Information (Optional):** Project file name or sanitized project title (e.g., `"MySong.rpp"` or `"MySong"`).
- **Transport & Playback State (Optional):** Current transport state (Playing, Recording, Paused, or Stopped) and project tempo (BPM).
- **Timeline & Session Elapsed Time (Optional):** Elapsed playback time, project timeline position, or total REAPER session uptime.
- **Project Structure (Optional):** Total track count in the active project tab.
- **Application Identification:** REAPER version identifier and ReaCord extension version.

---

## 3. How Information is Collected, Transmitted, and Stored

### 3.1 Local-Only Communication
- All data retrieved from REAPER is communicated **strictly on your local machine** using local Inter-Process Communication (IPC):
  - **Windows:** Named Pipe (`\\.\pipe\discord-ipc-0`)
  - **macOS / Linux:** UNIX domain socket (`/tmp/discord-ipc-0`)
- Data is passed directly from REAPER to your locally installed Discord desktop client.

### 3.2 Zero External Servers & Zero Telemetry
- ReaCord **does not** communicate with, host, or maintain any external web server, cloud database, tracking pixel, or analytics service.
- Neither the Developer nor any third party has access to your local REAPER session data through ReaCord.

### 3.3 No Collection of Sensitive or Audio Data
ReaCord **never** accesses, reads, processes, or transmits:
- Audio recordings, stems, waveforms, or microphone/input signals;
- MIDI data or musical compositions;
- Installed VST/AU/CLAP plugins or virtual instruments;
- Personal credentials, account passwords, or financial details;
- Personal Identifiable Information (PII) such as your legal name, physical address, or IP address.

### 3.4 Local Data Storage
- User configuration preferences (such as privacy toggles and display options) are stored locally on your machine within REAPER's standard configuration file (`reaper-extstate.ini`).
- No session data or presence history is permanently logged or stored by ReaCord.

---

## 4. Transmission to Discord

When ReaCord sends Rich Presence data to your local Discord desktop client, Discord transmits and broadcasts that activity to Discord's servers to display it on your public profile, according to your Discord account settings.

The handling, transmission, storage, and display of information by Discord is governed exclusively by **Discord's Privacy Policy**:  
👉 [https://discord.com/privacy](https://discord.com/privacy)

You can manage or disable Discord Activity Privacy directly within the Discord desktop client under **User Settings > Activity Privacy > Display current activity as a status message**.

---

## 5. Granular User Privacy Controls

ReaCord provides full user control over what information is visible. Through the ReaCord Configuration Window (**Extensions > ReaCord Configuration**) or ReaImGui script, you can customize:

| Setting | Options & Description |
|---|---|
| **Incognito Mode** | Instantly hides all project titles, details, and presence data from Discord. |
| **Project Title** | Choose between **Full Filename** (`Project.rpp`), **Name Only** (`Project`), **Generic Text** (*"Working on a Project"*), or **Completely Hidden**. |
| **Session Time** | Choose between **Project Elapsed Time**, **REAPER Uptime**, or **Hidden**. |
| **Playback State** | Choose between **Detailed** (*"Playing @ 120 BPM"*), **Simple** (*"Playing"*), or **Hidden**. |
| **Track Count** | Toggle display of active track count (*"• 24 Tracks"*). |

---

## 6. Data Retention and Deletion

- ReaCord retains **zero** session data. As soon as REAPER is closed or Discord is disconnected, local presence transmission ceases immediately.
- To completely remove all ReaCord configuration data from your machine, simply uninstall ReaCord via ReaPack or delete `reaper_reacord64.dll` (or OS equivalent) and remove the `[reaper_reacord]` section in `reaper-extstate.ini`.

---

## 7. Children's Privacy

ReaCord is a general-purpose DAW utility and does not knowingly collect, transmit, or solicit any personal data from children under the age of 13 (or under 16 in the European Union). Users must meet the minimum age required to maintain a Discord account in their jurisdiction.

---

## 8. Changes to This Privacy Policy

We may update this Privacy Policy from time to time. Any changes will be posted directly to the official ReaCord repository with an updated "Last Updated" date. Continued use of ReaCord after changes are published signifies your acceptance of the revised policy.

---

## 9. Contact & Inquiries

If you have questions, feedback, or concerns regarding this Privacy Policy or ReaCord's privacy practices, please contact us:

- **Author / Developer:** Bartek Staniak
- **GitHub Repository Issues:** [https://github.com/BartekStaniak/ReaCord/issues](https://github.com/BartekStaniak/ReaCord/issues)
- **Repository URL:** [https://github.com/BartekStaniak/ReaCord](https://github.com/BartekStaniak/ReaCord)
