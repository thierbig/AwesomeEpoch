# AwesomeEpoch
<img width="999" height="698" alt="image" src="https://github.com/user-attachments/assets/a647942a-7773-45b8-bfc3-fa80ec2063c2" />

AwesomeEpoch is a utility designed to enhance the **World of Warcraft** experience by launching the base **World of Warcraft 3.3.5a** client (`Wow.exe`) with the **AwesomeWotlkInjector**, enabling modern client features through custom CVar settings. This tool brings quality-of-life (QoL) improvements, such as the modern **Interaction Button** and the **Nameplates API**, allowing seamless integration with the **WeakAuras** addon for features like reactive spell anchoring (e.g., Overpower) and enemy castbars over nameplates. Users can customize the game executable path and perform manual injections if preferred. The injection method is safer from bans than the patched client method from vanilla awesome_wotlk.

## Features
- **Modern Interaction Button**: Adds a modern client-style interaction button for smoother gameplay, similar to modern WoW.
- **Advanced Nameplates API**: Enables the **WeakAuras** addon to:
  - Anchor reactive spells like Overpower for better visibility.
  - Display castbars over enemy nameplates for improved situational awareness.
- **Custom Game Path**: Set a custom path, folder, or filename for the client executable (defaults to `Wow.exe`) via `gameExeLocation.txt`, even if it’s outside default locations (e.g., Program Files, C:\Games).
- **Manual Injection Option**: Run the injector directly with a simple command for users who prefer manual control.
- **MSDF Vector Fonts**: Crisp, vector-based (MSDF) text rendering ported from upstream `awesome_wotlk`, controlled by the `MSDFMode` CVar (`0` = off, `1` = on, `2` = on incl. unsafe fonts). Requires `skia.dll`, which ships in the release but must be moved into your **game folder** (next to `Wow.exe`) — the client resolves it from there, not from the release folder.
- **Additional QoL Features**: Various enhancements to improve gameplay.

## Prerequisites
- The base **World of Warcraft 3.3.5a** client installed, with `Wow.exe` accessible (the tool also supports `Project-Epoch.exe` or `Ascension.exe` if that's what your install uses — see **Custom Game Path** below).
- **Addons** (required for full functionality):
  - **AwesomeEpochManager**: Enables custom CVar settings for modern features. (download below)
  - **PlateCastBarFixed**: Castbars on all nameplates
  - (Optional) WeakAuras: WeakAuras is a powerful and flexible framework that allows the display of highly customizable graphics on World of Warcraft's user interface to indicate buffs, debuffs, and other relevant information. NoM0Re version. (download below)
  - (Optional) Flash: For notifications on minimized icon bar
  - Download and install these addons in your game client's `Interface\AddOns` folder.

## Installation
1. **Download the Latest Release**:
   - Download AwesomeEpoch.zip from [one of the release](https://github.com/thierbig/AwesomeEpoch/releases)
   - Extract the AwesomeEpoch folder to a location of your choice (e.g., `C:\Users\YourName\AwesomeEpoch`).
   - Move the file **skia.dll** into the game client's folder next to other .dlls.

2. **Install Required Addons**:
   - Download **AwesomeEpochManager** from [releases](https://github.com/thierbig/AwesomeEpoch/releases/download/2.0/Addons.zip)
   - Download [PlateCastBarFixed](https://github.com/thierbig/PlateCastBar-3.3.5-Fixed/archive/refs/heads/main.zip)
   - (Optional) Download [WeakAuras](https://github.com/NoM0Re/WeakAuras-WotLK/archive/refs/heads/master.zip)
   - Place the addons in your game client's `Interface\AddOns` directory (e.g., `C:\Program Files\World of Warcraft\Interface\AddOns`).
     
3. **Optional: Set Custom Game Path**:
   - Edit `gameExeLocation.txt` in the extracted folder, and set a value.
   - Set it to the full path of your client executable, just its folder, or a bare filename (e.g., `E:\Games\epoch_live_bon`, `E:\Games\epoch_live_bon\Wow.exe`, or just `Wow.exe`).
   - Leave it commented (or delete the file) to use the default search order: `Wow.exe`, then `Project-Epoch.exe`, then `Ascension.exe`.

## Usage
AwesomeEpoch works by **injection** — the DLL is loaded into your running client. The default way is a single double-click; a manual command-prompt version is there if you want it.

### Double-Click (Default)
Once your client is configured, this is the whole flow:

1. (First time only) Point `gameExeLocation.txt` at your client — see **Set Custom Game Path** above. Leave it as-is to use the default search order (`Wow.exe`, then `Project-Epoch.exe`, then `Ascension.exe`).
2. **Double-click `AwesomeEpoch.exe`.** (If injection fails, right-click → *Run as administrator*.)

That's it. The launcher starts your client if it isn't already running, waits for it to load, and injects `AwesomeWotlkLib.dll` — no command line, no manual steps. It writes an `AutoInject_Awesome.log` next to itself if you need to check what happened.

### Manual Injection (Advanced)
Same injection, driven by hand instead of the launcher — handy if you start the client through some other launcher and just want to inject into it.

1. Start the game client yourself (via its launcher, or by running the exe directly) and let it fully load.
2. Open a Command Prompt as administrator and navigate to the release folder:
   ```
   cd C:\Users\YourName\AwesomeEpoch
   ```
3. Run the injector:
   ```
   AwesomeWotlkInjector.exe
   ```
   `AwesomeWotlkInjector.exe` reads `gameExeLocation.txt` (in the folder it runs from) to find the running client process and injects the CVar features into it.

### Interaction Button for handheld console
<img width="930" height="102" alt="image" src="https://github.com/user-attachments/assets/cdd0fc44-e796-4891-a722-1f5f1932e25d" />

### AwesomeEpochManager Addon
<img width="365" height="503" alt="image" src="https://github.com/user-attachments/assets/db57b337-9bff-4bc7-b82a-b134e7c18ed6" />
<img width="1216" height="911" alt="image" src="https://github.com/user-attachments/assets/674fb2f8-3efb-47c7-81d9-6394d6338248" />


### Enabling Castbars
To enable castbars over all enemy nameplates, you need to install [PlateCastBarFixed](https://github.com/thierbig/PlateCastBar-3.3.5-Fixed/archive/refs/heads/main.zip)

## Troubleshooting
- **Injection not working**:
  - Ensure `gameExeLocation.txt` contains a valid path to your game client executable or its folder, or leave it commented for default paths.
  - Run `AwesomeEpoch.exe` (or `AwesomeWotlkInjector.exe`) as administrator.
  - Check `AutoInject_Awesome.log` in the release folder for what the launcher tried and where it failed.
- **Injection fails**:
  - For manual injection, verify the game client is running before injecting.
  - Make sure `AwesomeWotlkLib.dll` is in the same folder as `AwesomeEpoch.exe`.
  - Make sure `skia.dll` is in your **game folder** (next to `Wow.exe`), not in the release folder — `AwesomeWotlkLib.dll` imports it, so injection fails without it there.
  - Ensure you have the required addons (`AwesomeEpochManager`, `Flash`, `WeakAuras`).
  

## Notes
- **Run as Administrator**: Always run `AwesomeEpoch.exe` (or `AwesomeWotlkInjector.exe`) with administrator privileges to avoid permission issues.
- **Support**: For issues or feature requests, create an issue on the [GitHub Issues](https://github.com/thierbig/AwesomeEpoch/issues) page.

## Credits
AwesomeEpoch is built upon the **awesome_wotlk** project by **[someweirdhuman](https://github.com/someweirdhuman)**, **[NoM0Re](https://github.com/NoM0Re)** and **[FrostAtom](https://github.com/FrostAtom)**. Their work on the [awesome_wotlk](https://github.com/someweirdhuman/awesome_wotlk) patcher enables the modern CVar features that make this tool possible. Thank you for their contributions to the WoW community!

## Other Mods I maintain
- [PlateCastBarFixed](https://github.com/thierbig/PlateCastBar-3.3.5-Fixed)
- [EpochAuthenticHD](https://github.com/thierbig/EpochAuthenticHD)
- [AddonList-Fixed](https://github.com/thierbig/AddonList-3.3.5-Fixed)
- [Auctionator-Fixed](https://github.com/thierbig/Auctionator-3.3.5-Fixed)
- [LossOfControlSmart](https://github.com/thierbig/LossOfControlSmart-3.3.5)
- [TrinketMenu](https://github.com/thierbig/TrinketMenu-3.3.5)

## Weakauras
- Feel free to use my other custom Epoch Weakauras: [https://wago.io/p/thierbig](https://wago.io/p/thierbig)

## License
This project is licensed under the MIT License.
