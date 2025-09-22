# AwesomeEpoch
<img width="999" height="698" alt="image" src="https://github.com/user-attachments/assets/a647942a-7773-45b8-bfc3-fa80ec2063c2" />

**WARNING: USING THIS TOOL MIGHT RESULT IN A BAN**

AwesomeEpoch is a utility designed to enhance the **World of Warcraft: Ascension** experience by launching `Ascension.exe` with the **AwesomeWotlkInjector**, enabling modern client features through custom CVar settings. This tool brings quality-of-life (QoL) improvements, such as the modern **Interaction Button** and the **Nameplates API**, allowing seamless integration with the **WeakAuras** addon for features like reactive spell anchoring (e.g., Overpower) and enemy castbars over nameplates. Users can customize the game executable path and perform manual injections if preferred. The injection method is safer from bans than the patched client method from vanilla awesome_wotlk.

## Features
- **Modern Interaction Button**: Adds a modern client-style interaction button for smoother gameplay, similar to modern WoW.
- **Advanced Nameplates API**: Enables the **WeakAuras** addon to:
  - Anchor reactive spells like Overpower for better visibility.
  - Display castbars over enemy nameplates for improved situational awareness.
- **Custom Game Path**: Set a custom path for `Ascension.exe` via `gameExeLocation.txt`, even if it’s outside default locations (e.g., Program Files, C:\Games).
- **Manual Injection Option**: Run the injector directly with a simple command for users who prefer manual control.
- **Additional QoL Features**: Various enhancements to improve gameplay (note: changing nameplate distance is disabled as it’s considered cheating).
- **Logging**: Detailed logs in `AutoInject_Awesome.log` and `injector_output.log` for troubleshooting.

## Prerequisites
- **World of Warcraft: Ascension** installed, with `Ascension.exe` accessible.
- **Addons** (required for full functionality):
  - **AwesomeCVar**: Enables custom CVar settings for modern features. (download below)
  - **Flash**: Supports additional visual enhancements.(download below)
  - **WeakAuras**: WeakAuras is a powerful and flexible framework that allows the display of highly customizable graphics on World of Warcraft's user interface to indicate buffs, debuffs, and other relevant information. NoM0Re version. (download below)
  - Download and install these addons in your Ascension `Interface\AddOns` folder.
- **WeakAuras Addon**: Required for nameplate castbars and reactive spell anchoring.
  - Import the WeakAura for castbars: [https://wago.io/Xm56P0I81](https://wago.io/Xm56P0I81).

## Installation
1. **Download the Latest Release**:
   - Download AwesomeEpoch from [latest release](https://github.com/thierbig/AwesomeEpoch/releases/download/1.0/AwesomeEpoch.zip)
   - Extract the AwesomeEpoch folder to a location of your choice (e.g., `C:\Users\YourName\AwesomeEpoch`).

2. **Install Required Addons**:
   - Download **AwesomeCVar** and **Flash** addons made for [awesome_wotlk](https://github.com/someweirdhuman/awesome_wotlk/releases/download/v30/addons_v30.zip)
   - Place the addons in your Ascension `Interface\AddOns` directory (e.g., `C:\Program Files\Ascension Launcher\resources\epoch_live\Interface\AddOns`).
   - Install [WeakAuras](https://github.com/NoM0Re/WeakAuras-WotLK) addon and import the castbar WeakAura:
     - Visit [https://wago.io/Xm56P0I81](https://wago.io/Xm56P0I81).
     - Copy the import string and paste it into WeakAuras in-game (`/wa` to open).

3. **Optional: Set Custom Game Path**:
   - Edit `gameExeLocation.txt` in the extracted folder,remove the `#' and set a new path.
   - Add the path to your `Ascension.exe` or its folder (e.g., `E:\Games\epoch_live_bon` or `E:\Games\epoch_live_bon\Ascension.exe`).
   - Leave the file empty or delete it to use default paths (e.g., Program Files, C:\Games, D:\Games).

## Usage
### Automated Injection
1. Run `AwesomeEpoch.exe`:
   - Double-click `AwesomeEpoch.exe` in the release folder.
2. The tool will:
   - Read `gameExeLocation.txt` to locate `Ascension.exe` or probe default paths.
   - Launch `Ascension.exe` if it’s not running.
   - Wait 4 seconds, then inject `AwesomeWotlkInjector.exe` to apply CVar settings.
   - Retry up to 3 times if injection fails, checking for success via logs.

### Manual Injection
For users who prefer manual control:
1. Ensure `Ascension.exe` is running (start it via the game launcher or manually).
2. Open a Command Prompt as administrator and navigate to the release folder:
   ```
   cd C:\Users\YourName\AwesomeEpoch
   ```
3. Run the injector directly:
   ```
   AwesomeWotlkInjector.exe Ascension.exe
   ```

### Interaction Button for handheld console
<img width="930" height="102" alt="image" src="https://github.com/user-attachments/assets/cdd0fc44-e796-4891-a722-1f5f1932e25d" />

### Awesome CVar Addon
<img width="328" height="251" alt="image" src="https://github.com/user-attachments/assets/f1c3b8a9-10ec-46de-8cd9-5a184577c8af" />
<img width="1205" height="906" alt="image" src="https://github.com/user-attachments/assets/f74a2725-6341-4650-88f3-952012660264" />

### Enabling Castbars
To enable castbars over all enemy nameplates:
1. In-game, open WeakAuras (`/wa`).
2. Import the WeakAura from: [https://wago.io/Xm56P0I81](https://wago.io/Xm56P0I81).
3. Ensure the **AwesomeCVar** and **WeakAuras** addons are enabled.

## Troubleshooting
- **AwesomeEpoch.exe not working**:
  - Check `AutoInject_Awesome.log` and `injector_output.log` in the release folder for errors.
  - Ensure `gameExeLocation.txt` contains a valid path to `Ascension.exe` or its folder, or leave it empty for default paths.
  - Run `AwesomeEpoch.exe` or `AutoInject_Awesome.bat` as administrator.
- **Injection fails**:
  - Verify `Ascension.exe` is running before manual injection.
  - Check if `AwesomeWotlkInjector.exe` is in the release folder or the `AwesomeWotlkInjector` subfolder.
  - Ensure you have the required addons (`AwesomeCVar`, `Flash`, `WeakAuras`).
- **Castbars or WeakAuras not working**:
  - Confirm the WeakAura is imported correctly and addons are enabled.
  - Reload the UI (`/reload`) or restart the game.

## Notes
- **Nameplate Distance Limitation**: Changing nameplate distance is disabled as it’s considered cheating.
- **Run as Administrator**: Always run `AwesomeEpoch.exe`  or manual injection commands with administrator privileges to avoid permission issues.
- **Logs**: Check logs files `AutoInject_Awesome.log` and `injector_output.log` to diagnose issues.
- **Support**: For issues or feature requests, create an issue on the [GitHub Issues](https://github.com/YourUsername/AwesomeEpoch/issues) page.

## Credits
AwesomeEpoch is built upon the **awesome_wotlk** project by **[someweirdhuman](https://github.com/someweirdhuman)** and **[FrostAtom](https://github.com/FrostAtom)**. Their work on the [awesome_wotlk](https://github.com/someweirdhuman/awesome_wotlk) patcher enables the modern CVar features that make this tool possible. Thank you for their contributions to the WoW community!

## Other Mods I maintain
- [AddonList](https://github.com/thierbig/AddonList-3.3.5-Fixed)
- [EpochAuthenticHD](https://github.com/thierbig/EpochAuthenticHD)

## Weakauras
- Feel free to use my other custom Epoch Weakauras: [https://wago.io/p/thierbig](https://wago.io/p/thierbig)

## License
This project is licensed under the MIT License.
