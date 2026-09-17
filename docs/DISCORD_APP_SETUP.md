> [!NOTE]  
> **ReaCord works 100% out-of-the-box!**  
> ReaCord is pre-configured with the official REAPER Discord Application ID (`1462972195658534965`). You do **not** need to follow this guide unless you want custom application branding, custom names, or custom logos.

---

## Step 1: Create a Discord Application (Optional)

1. Open your browser and navigate to the **[Discord Developer Portal](https://discord.com/developers/applications)**.
2. Log in with your Discord account.
3. Click the **New Application** button in the top-right corner.
4. Enter a name for the application. This name will appear on your Discord profile as the activity title (for example: **REAPER** or **Cockos REAPER**).
5. Accept Discord's Developer Terms of Service and click **Create**.

---

## Step 2: Application Details & Verification (Terms & Privacy)

1. In the left navigation menu, select **General Information**.
2. Under the application name, find the **Application ID** (also called Client ID) and copy it.
3. For Discord App Verification and profile compliance, paste the following URLs into the respective fields:
   - **Terms of Service URL:**
     ```text
     https://github.com/BartekStaniak/ReaCord/blob/main/TERMS_OF_SERVICE.md
     ```
   - **Privacy Policy URL:**
     ```text
     https://github.com/BartekStaniak/ReaCord/blob/main/PRIVACY_POLICY.md
     ```
4. Click **Save Changes** at the bottom of the page.

---

## Step 3: Configure Rich Presence Art Assets

Pre-rendered 512x512 transparent PNG assets are provided in this repository under the [`assets/discord/`](../assets/discord) folder:
- `assets/discord/reacord_logo.png` (hybrid emblem)
- `assets/discord/reaper_logo.png` (classic REAPER pick)
- `assets/discord/play.png`
- `assets/discord/record.png`
- `assets/discord/pause.png`
- `assets/discord/stop.png`

To upload them to your Discord application:

1. In the left navigation menu of the Developer Portal, go to **Rich Presence** > **Art Assets**.
2. Click **Add Image(s)** and upload each asset with its exact key name:
   - **Large Image Asset**:
     - Key Name: `reaper_logo` (Upload `assets/discord/reacord_logo.png` or `assets/discord/reaper_logo.png`)
   - **Small Badge Assets**:
     - Key Name: `play` (Upload `assets/discord/play.png`)
     - Key Name: `record` (Upload `assets/discord/record.png`)
     - Key Name: `pause` (Upload `assets/discord/pause.png`)
     - Key Name: `stop` (Upload `assets/discord/stop.png`)
3. Click **Save Changes** at the bottom of the page.

> **Note**: Discord may take up to 5–10 minutes to propagate newly uploaded art assets across its global CDN.

---

## Step 4: Configure ReaCord in REAPER

1. Open REAPER.
2. Open ReaCord Settings using either:
   - REAPER Action List (`?`) -> search `ReaCord: Open Settings...`
   - Or run the ReaImGui script: `ReaCord_Settings_ImGui.lua`
3. Paste your **Application ID** into the **Discord Client ID** field.
4. Click **Apply** or **OK**.

ReaCord will automatically connect to your local Discord desktop client and broadcast your customized Rich Presence!
