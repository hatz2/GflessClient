# Metin2 Setup Guide - GflessClient

This guide will help you set up GflessClient to work with Metin2.

## 📋 Requirements

1. **Fiddler** - For capturing authentication data
2. **Gameforge Client** - Must be installed
3. **Metin2 Game** - Installed via Gameforge Client
4. **GflessClient** - This application

---

## 🔧 Step 1: Extract Blackbox (TRA)

### 1.1 Install and Configure Fiddler

1. Download [Fiddler Classic](https://www.telerik.com/download/fiddler)
2. Install and open Fiddler
3. Go to **Tools** → **Options** → **HTTPS**
4. Check **"Decrypt HTTPS traffic"**
5. Click **OK** (install certificates if prompted)

### 1.2 Capture Metin2 Authentication

1. **Start Fiddler** (must be running before Gameforge Client)
2. **Open Gameforge Client**
3. **Login to your Gameforge account**
4. **Start Metin2** (select a character and enter game)
5. **In Fiddler**, look for requests to:
   ```
   POST https://spark.gameforge.com/api/v1/auth/iovation
   ```

6. **Click on that request** → Go to **Inspectors** → **WebForms** or **TextView**
7. **Find the "blackbox" parameter** - it starts with `"tra:"`
8. **Copy the entire blackbox value** (example: `tra:JVqc0...`)

---

## 🔐 Step 2: Generate Identity File

1. **Open GflessClient**
2. Go to **Options** → **Identity Generator**
3. **Paste your blackbox** (the tra:... string you copied)
4. Click **"Generate and Save"**
5. **Save the identity file** somewhere safe (e.g., `metin2_identity.json`)

---

## ⚙️ Step 3: Configure GflessClient

1. Go to **Options** → **Settings**
2. **Select your identity file** (the one you just generated)
3. **Select your metin2client.exe**:
   - Usually located at:
     ```
     C:\Users\[YOUR_NAME]\AppData\Local\Gameforge\Games\TUR_tur\Metin2\metin2client.exe
     ```
   - Or wherever you installed Metin2

4. **Installation ID** (optional):
   - GflessClient will try to auto-detect from registry
   - If not found, you can manually enter it

---

## 👤 Step 4: Add Gameforge Account

1. Click **"Add Account"** in GflessClient
2. Enter your **Gameforge email**
3. Enter your **Gameforge password**
4. Select the **identity file** you created
5. **(Optional)** Configure proxy settings if needed
6. Click **Add**

**Note:** The app will automatically detect Metin2 based on your client path (metin2client.exe).

---

## 🎮 Step 5: Add Game Accounts

After authentication succeeds:

1. Your **Metin2 game accounts** will be listed
2. Select which accounts you want to use
3. Configure for each account:
   - **Server** (which server to connect to)
   - **Channel** (which channel)
   - **Character slot** (which character to select)

---

## ▶️ Step 6: Launch Metin2

1. Select the accounts you want to open
2. Click **Play**
3. GflessClient will:
   - Authenticate with Gameforge
   - Get game tokens
   - Launch metin2client.exe
   - (Optional) Auto-select character if DLL injection is configured

---

## 🚨 Troubleshooting

### Issue: "Couldn't find InstallationId"

**Solution:**
- Open Gameforge Client at least once
- Or manually extract Installation ID from Windows Registry:
  ```
  HKEY_CURRENT_USER\SOFTWARE\Gameforge4d\GameforgeClient\MainApp
  ```
  Look for `InstallationId` value

### Issue: "Authentication failed"

**Possible causes:**
1. **Blackbox expired** - Extract a fresh one from Fiddler
2. **Wrong email/password** - Double check credentials
3. **Captcha required** - GflessClient will show captcha dialog
4. **Identity file corrupted** - Regenerate identity file

### Issue: "Game doesn't launch"

**Check:**
1. **Correct path to metin2client.exe**
2. **Gameforge Client is closed** (can conflict)
3. **Run as Administrator** (GflessClient requires admin for DLL injection)
4. **Antivirus not blocking** Injector.exe or GflessDLL.dll

### Issue: "Character not auto-selected"

**Note:** Auto-selection requires:
- DLL injection (advanced feature)
- Metin2-specific game structures (not yet fully implemented)

For now, you'll need to manually select character after launch.

---

## 📊 Collected Information Reference

During setup, you collected these IDs:

| Information | Your Value | Purpose |
|-------------|------------|---------|
| **Game ID** | `fab180a3-cd65-4b7e-bd0e-2ef77fd0c258.tr-TR` | Identifies Metin2 in Gameforge API |
| **Environment ID** | `5401ee5b-1316-41ae-a628-73377b8676ba` | Metin2 game environment |
| **Application ID** | `d3b2a0c1-f0d0-4888-ae0b-1c5e1febdafb` | Client application identifier |
| **Executable** | `metin2client.exe` | Game executable name |

These are **hardcoded** in the application and match your Metin2 region (TR).

---

## 🎯 Advanced: Multiple Accounts

You can launch multiple Metin2 clients simultaneously:

1. **Create profiles** with different game accounts
2. **Select multiple accounts** in the UI
3. **Click Play** - all will launch at once

Each account can have:
- Different servers
- Different channels
- Different characters
- Different proxy settings
- Different identity files (if needed)

---

## ⚠️ Important Notes

1. **TOS Compliance**: Using this tool may violate Gameforge Terms of Service. Use at your own risk.
2. **Account Security**: Keep your identity files secure - they contain authentication data.
3. **Blackbox Expiration**: Blackboxes may expire. If auth fails, generate a new identity file.
4. **Updates**: Gameforge may change their API - updates to GflessClient may be needed.

---

## 💡 Tips

- **Save multiple identity files** for backup
- **Test with one account** before adding multiple
- **Close Gameforge Client** before using GflessClient
- **Use proxy per account** if you want different IPs
- **Check Discord** for community support (see README.md)

---

## 📞 Support

If you have issues:
1. Check this guide again
2. Check the main README.md
3. Join the Discord server (link in README.md)
4. Check GitHub issues

---

**Enjoy playing Metin2 with GflessClient!** 🎮
