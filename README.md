# GflessClient

This application simulates almost everything that the Gameforge client does allowing you to have multiple gameforge accounts in the same launcher and open several game clients with just one click.

**Supported Games:**
- NosTale (original support)
- Metin2 (TR region) - [See Metin2 Setup Guide](METIN2_SETUP_GUIDE.md)

# Discord

To get help and check out more in depths tutorials you can join the discord server.

[<img src="https://discord.com/api/guilds/1339601581049647136/widget.png?style=banner2">](https://discord.gg/AVs6g3myx3)

## Features

* You can have multiple gameforge accounts in the same launcher.
* You can log in with your gameforge accounts through a proxy server.
* You can use custom game clients for each gameforge account in case you want to proxy the game aswell.
* You can use a different identity and installation id for each gameforge account.
* The software can check for game updates at startup
* You can disable the nosmall popup at first daily login.
* You can create new game accounts.
* Allows you to create profiles with game accounts that belong to different gameforge accounts and set a custom name for each account.
* You can selet and open multiple game accounts in one click.
* The process of selecting the server, channel and the character is fully automated.
* Allows you to quickly open the game settings by clicking in the wheel button.
* Allows you to change the game language.

## Instructions

### For NosTale:

1. Download the latest release version
2. Extract the folder and open GflessClient.exe
3. Generate your identity file (See instructions below)
4. Go to Options > Settings and select your identity file.
5. Go to Options > Settings and select your NostaleClientX.exe
6. Add a gameforge account
7. Select the accounts you want to open and click on Play

### For Metin2:

**See the [Metin2 Setup Guide](METIN2_SETUP_GUIDE.md) for detailed instructions.**

Quick steps:
1. Extract blackbox using Fiddler (from `auth/iovation` endpoint)
2. Generate identity file using the blackbox
3. Configure GflessClient with identity file and metin2client.exe path
4. Add your Gameforge account
5. Launch Metin2 accounts

## How to generate identity file

You'll need to extract a valid blackbox from the request to auth/iovation following this steps:

1. Open [Fiddler](https://www.telerik.com/download/fiddler-b) and the Gameforge Client.
2. Enable HTTPS traffic decryption / Tools > Options > HTTPS > Decrypt HTTPS traffic.
3. Connect your game account and copy the blackbox from the request to auth/iovation (the blackbox look something like "tra:JVqc0...")
4. In the Gfless Client go to Options > Identity generator, paste the blackbox and click on the button that says "Generate and save".
5. Save the generated identity to a file.

## Credits

Big thanks and full credits to [morsisko](https://github.com/morsisko) and [stdLemon](https://github.com/stdLemon) for all the reverse engineering needed to make this project possible.<br>
Repositories used for this project: [Nostale-Auth](https://github.com/morsisko/NosTale-Auth), [Nostale-Gfless](https://github.com/morsisko/NosTale-Gfless) and [nostale-auth](https://github.com/stdLemon/nostale-auth)
