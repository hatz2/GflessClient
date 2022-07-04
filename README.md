# GflessClient

This application simulates almost everything that the Gameforge client does allowing you to have multiple gameforge accounts in the same launcher and open several game clients with just one click.

## Features

* You can have multiple gameforge accounts in the same launcher.
* Allows you to create profiles and rename your accounts.
* You can selet and open multiple game accounts in one click.
* The process of selecting the server, channel and the character is fully automated.
* Allows you to quickly open the game settings by clicking in the wheel button.
* Allows you to change the game language.

## Instructions

1. Download the latest release version
2. Extract the folder and open GflessClient.exe
3. Generate your identity file (See instructions below)
4. Go to Options > Settings and select your identity file.
5. Go to Options > Settings and select your NostaleClientX.exe<br> 
![](https://cdn.discordapp.com/attachments/913546241370112010/993510428070137896/unknown.png)
4. Add a gameforge account<br>
![](https://media.discordapp.net/attachments/739413308192325633/898543778627989524/unknown.png)
5. Select the accounts you want to open and click on Play<br>
![](https://media.discordapp.net/attachments/739413308192325633/898544878450012200/unknown.png)

## How to generate identity file

You'll need to extract a valid blackbox from the request to auth/iovation following this steps:

1. Open [Fiddler](https://www.telerik.com/download/fiddler-b) and the Gameforge Client.
2. Enable HTTPS traffic decryption / Tools > Options > HTTPS > Decrypt HTTPS traffic.<br>![](https://cdn.discordapp.com/attachments/913546241370112010/993508702713823302/unknown.png)
3. Connect your game account and copy the blackbox from the request to auth/iovation (the blackbox look something like "tra:JVqc0...")<br>![](https://cdn.discordapp.com/attachments/913546241370112010/993509372586106930/unknown.png)
4. In the Gfless Client go to Options > Identity generator, paste the blackbox and click on the button that says "Generate and save".
5. Save the generated identity to a file.

## Credits

Big thanks and full credits to [morsisko](https://github.com/morsisko) and [stdLemon](https://github.com/stdLemon) for all the reverse engineering needed to make this project possible.<br>
Repositories used for this project: [Nostale-Auth](https://github.com/morsisko/NosTale-Auth), [Nostale-Gfless](https://github.com/morsisko/NosTale-Gfless) and [nostale-auth](https://github.com/stdLemon/nostale-auth)
