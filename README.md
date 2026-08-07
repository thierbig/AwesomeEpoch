# AwesomeEpoch
<img width="999" height="698" alt="image" src="https://github.com/user-attachments/assets/a647942a-7773-45b8-bfc3-fa80ec2063c2" />

**WARNING: USING THIS TOOL MIGHT RESULT IN A BAN**

AwesomeEpoch is a utility designed to enhance the **World of Warcraft** experience by launching the base **World of Warcraft 3.3.5a** client (`Wow.exe`) with the **AwesomeWotlkInjector**, enabling modern client features through custom CVar settings. This tool brings quality-of-life (QoL) improvements, such as the modern **Interaction Button** and the **Nameplates API**, allowing seamless integration with the **WeakAuras** addon for features like reactive spell anchoring (e.g., Overpower) and enemy castbars over nameplates. Users can customize the game executable path and perform manual injections if preferred. The injection method is safer from bans than the patched client method from vanilla awesome_wotlk.

## Features
- **Modern Interaction Button**: Adds a modern client-style interaction button for smoother gameplay, similar to modern WoW.
- **Advanced Nameplates API**: Enables the **WeakAuras** addon to:
  - Anchor reactive spells like Overpower for better visibility.
  - Display castbars over enemy nameplates for improved situational awareness.
- **Custom Game Path**: Set a custom path, folder, or filename for the client executable (defaults to `Wow.exe`) via `gameExeLocation.txt`, even if it’s outside default locations (e.g., Program Files, C:\Games).
- **Manual Injection Option**: Run the injector directly with a simple command for users who prefer manual control.
- **MSDF Vector Fonts**: Crisp, vector-based (MSDF) text rendering ported from upstream `awesome_wotlk`, controlled by the `MSDFMode` CVar (`0` = off, `1` = on, `2` = on incl. unsafe fonts). Requires `skia.dll`, which ships in the release and must sit in the game folder next to `AwesomeWotlkLib.dll` (the patcher copies it automatically).
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

2. **Install Required Addons**:
   - Download **AwesomeEpochManager** from [releases](https://github.com/thierbig/AwesomeEpoch/releases/download/2.0/Addons.zip)
   - Download [PlateCastBarFixed](https://github.com/thierbig/PlateCastBar-3.3.5-Fixed/archive/refs/heads/main.zip)
   - (Optional) Download [WeakAuras](https://github.com/NoM0Re/WeakAuras-WotLK/archive/refs/heads/master.zip)
   - Place the addons in your game client's `Interface\AddOns` directory (e.g., `C:\Program Files\World of Warcraft\Interface\AddOns`).
   - 
3. **Optional: Set Custom Game Path**:
   - Edit `gameExeLocation.txt` in the extracted folder, remove the leading `#` and set a value.
   - Set it to the full path of your client executable, just its folder, or a bare filename (e.g., `E:\Games\epoch_live_bon`, `E:\Games\epoch_live_bon\Wow.exe`, or just `Wow.exe`).
   - Leave it commented (or delete the file) to use the default search order: `Wow.exe`, then `Project-Epoch.exe`, then `Ascension.exe`.

## Usage
### Patching (Default)
The main release zip ships `AwesomeWotlkPatch.exe`, `AwesomeWotlkLib.dll`, and `gameExeLocation.txt`, so this is the flow that works right out of the downloaded release with no extra steps.
1. Open a Command Prompt as administrator and navigate to the release folder:
   ```
   cd C:\Users\YourName\AwesomeEpoch
   ```
2. Run the patcher:
   ```
   AwesomeWotlkPatch.exe
   ```
   `AwesomeWotlkPatch.exe` reads `gameExeLocation.txt` (in the folder it runs from) to locate the client executable and patches it directly. The value can be a full path, just a folder, or a bare filename, and defaults to `Wow.exe` if left unset.

### Injection (Alternative / Advanced)
Instead of patching the executable, you can inject the CVar settings into a client that's already running, using `AwesomeWotlkInjector.exe`. Note: **the injector is not included in the main release zip** — it's built separately by the `build-injector.yml` workflow, so you'll need to grab it from that workflow's build artifacts if you want to use this method.
1. Start the game client yourself (via its launcher, or by running the exe directly) and let it fully load.
2. Open a Command Prompt as administrator and navigate to the folder containing `AwesomeWotlkInjector.exe`:
   ```
   cd C:\Users\YourName\AwesomeEpoch
   ```
3. Run the injector:
   ```
   AwesomeWotlkInjector.exe
   ```
   `AwesomeWotlkInjector.exe` reads `gameExeLocation.txt` (in the folder it runs from) to find the running client process and injects the CVar settings into it.

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
  - Run `AwesomeWotlkInjector.exe` or `AwesomeWotlkPatch.exe` as administrator.
- **Injection fails**:
  - Verify the game client is running before injecting.
  - Check if `AwesomeWotlkInjector.exe` is in the release folder or the `AwesomeWotlkInjector` subfolder.
  - Ensure you have the required addons (`AwesomeEpochManager`, `Flash`, `WeakAuras`).
  

## Notes
- **Run as Administrator**: Always run `AwesomeWotlkInjector.exe` or `AwesomeWotlkPatch.exe` with administrator privileges to avoid permission issues.
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
