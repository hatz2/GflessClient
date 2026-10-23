# GflessClient

This application simulates almost everything the Gameforge client does, allowing you to manage multiple Gameforge accounts in the same launcher and open several game clients with a single click.

# Discord

For help and more in-depth tutorials, join the Discord server.

[<img src="https://discord.com/api/guilds/1339601581049647136/widget.png?style=banner2">](https://discord.gg/AVs6g3myx3)

## Features

### Account Management
* Store multiple Gameforge accounts in the same launcher.
* Log in with Gameforge credentials or an existing authentication token.
* Use a different identity (fingerprint) and installation ID for each Gameforge account.
* Add accounts with SOCKS5 proxy support (per-account IP, port, username, password).

### Profile System
* Create named profiles containing game accounts from different Gameforge accounts.
* Set custom pseudonyms (display names) for each game account.
* Reorder accounts within a profile.
* Import and export profiles to `.ini` files.
* Batch edit login settings for multiple selected accounts at once.

### Automated Launching
* Select and open multiple game accounts in one click.
* Configurable delay between launching each client.
* Fully automated server, channel, and character selection.
* Auto-login with configurable defaults.

### Game Client Management
* Use a custom game client path per Gameforge account.
* Check for game updates at startup (optional).
* Manual game repair via the repair button.
* Automatically detects and offers to kill a running Gameforge client to avoid conflicts.
* Open game settings (NtConfig.exe) with the settings wheel button.
* Change the game language.

### In-Game Features (via DLL injection)
* Disable the NosMall popup on the first daily login.
* Automated server/channel/character selection through the injected DLL.

### Captcha Handling
* Built-in captcha dialog supporting image-drop captchas.
* Retry loop for incorrect answers.
* The `gf-pow-captcha` check (Gameforge's own script doing a proof of work in a browser) runs by itself through the bundled `PowCaptchaSolver.exe`, in a small Edge/Chrome window routed through the account's proxy.

### Identity Generator
* Generate identity files from iovation blackbox strings.
* Decode blackbox data into a JSON identity file.

### Interface
* Five UI themes: the native Modern Windows (Windows 11), Fusion and Windows (Windows Vista) looks, plus custom Dark and Light themes.
* Minimizes to system tray instead of closing.
* Single-instance enforcement.

## Instructions

1. Download and run the latest release installer.
2. Follow the installer prompts — it will create start menu entries and desktop shortcuts.
3. Generate your identity file (see instructions below).
4. Go to `Options > Settings` and select your identity file.
5. Go to `Options > Settings` and select your `NostaleClientX.exe`.
6. Add a Gameforge account.
7. Select the accounts you want to open and click **Play**.

## How to generate identity file

You need to extract a valid blackbox from a request to `auth/iovation`. The easiest way is to use `BlackboxExtractor.exe`, which is bundled in the installation directory:

1. The official Gameforge client can be running already — BlackboxExtractor will attach to it. If it's not running, BlackboxExtractor will launch it automatically.
2. Run `BlackboxExtractor.exe` from the installation folder (default: `C:\Program Files\Gfless Client`).
3. It will automatically launch the Gameforge client and capture the blackbox from the `auth/iovation` request.
4. The blackbox string is copied to your clipboard automatically. You should see it in the console output.
5. In GflessClient, go to `Options > Identity generator`, paste the blackbox, and click **Generate and save**.
6. Save the generated identity JSON file to a location of your choice.

> **Note:** If you prefer to capture the blackbox manually with Fiddler, you can still do so — but the bundled BlackboxExtractor is the recommended method.

## Credits

Big thanks and full credits to [morsisko](https://github.com/morsisko) and [stdLemon](https://github.com/stdLemon) for all the reverse engineering that made this project possible.<br>
Repositories used for this project: [NosTale-Auth](https://github.com/morsisko/NosTale-Auth), [NosTale-Gfless](https://github.com/morsisko/NosTale-Gfless) and [nostale-auth](https://github.com/stdLemon/nostale-auth)
