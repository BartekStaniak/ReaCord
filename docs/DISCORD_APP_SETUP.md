# Discord Application Setup Guide for ReaCord

This guide explains how to set up your own Discord Application to provide customized artwork, badges, and titles for your REAPER Discord Rich Presence integration.

---

## Step 1: Create a Discord Application

1. Open your browser and navigate to the **[Discord Developer Portal](https://discord.com/developers/applications)**.
2. Log in with your Discord account.
3. Click the **New Application** button in the top-right corner.
4. Enter a name for the application. This name will appear on your Discord profile as the activity title (for example: **REAPER** or **Cockos REAPER**).
5. Accept Discord's Developer Terms of Service and click **Create**.

---

## Step 2: Copy your Client ID

1. In the left navigation menu, select **General Information**.
2. Under the application name, find the **Application ID** (also called Client ID).
3. Click **Copy** to copy this numerical ID to your clipboard.

---

## Step 3: Configure Rich Presence Art Assets

To show the REAPER logo and transport icons (Play, Record, Pause, Stop) on your Discord profile:

1. In the left navigation menu, go to **Rich Presence** > **Art Assets**.
2. Click **Add Image(s)** to upload your assets:
   - **Large Image Asset**:
     - Key Name: `reaper_logo`
     - Recommended Dimensions: 512x512 or 1024x1024 PNG (Square, transparent background).
   - **Small Badge Assets**:
     - Key Name: `play` (Icon representing playback state)
     - Key Name: `record` (Red circle or badge representing recording)
     - Key Name: `pause` (Pause badge)
     - Key Name: `stop` (Square badge representing stopped/idle state)
3. Click **Save Changes** at the bottom of the page.

> **Note**: Discord may take up to 5–10 minutes to propagate newly uploaded art assets across its CDN.

---

## Step 4: Configure ReaCord in REAPER

1. Open REAPER.
2. Open ReaCord Settings using either:
   - REAPER Action List (`?`) -> search `ReaCord: Open Settings...`
   - Or run the ReaImGui script: `ReaCord_Settings_ImGui.lua`
3. Paste your **Application ID** into the **Discord Client ID** field.
4. Click **Apply** or **OK**.

ReaCord will automatically connect to your local Discord desktop client and broadcast your customized Rich Presence!
