# 3DS CloudSaver ☁️🎮

A cloud save manager for the Nintendo 3DS. Synchronize your save files across multiple 3DS consoles using a companion server.

![Platform](https://img.shields.io/badge/platform-Nintendo%203DS-red)
![License](https://img.shields.io/badge/license-MIT-blue)
![Language](https://img.shields.io/badge/language-C-blue)

## ✨ Features

- **Cloud Save Sync** – Upload and download save files for any installed game
- **Multi-Device Support** – Share saves between multiple 3DS consoles
- **Sync All** – One-tap sync for all games with commit messages
- **Game Browser** – Beautiful top-screen grid showing all installed game covers
- **Save Details** – View creation date, device name, region, and custom descriptions
- **Multi-Language** – English, Deutsch, Français, Español, Italiano, 日本語
- **Multi-Region** – EUR, USA, JPN, KOR support
- **Persistent Config** – Device name, server settings, and preferences saved locally
- **Auto-Connect** – Automatically connects to your server on app launch

## 🗂️ Cloud Storage Structure

```
/DEVICENAME/GAMENAME/GAMEID/SAVEFILES/
├── MyDevice_20260401_120000.sav
├── MyDevice_20260401_120000.meta
├── OtherDevice_20260330_180000.sav
└── OtherDevice_20260330_180000.meta
```

## 📋 Requirements

### Build Requirements
- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM
- [libctru](https://github.com/devkitPro/libctru)
- [citro2d](https://github.com/devkitPro/citro2d) & citro3d
- [makerom](https://github.com/3DSGuy/Project_CTR/releases) (for CIA building)
- [bannertool](https://github.com/Steveice10/bannertool/releases) (for CIA building)

### Runtime Requirements
- Nintendo 3DS with CFW (Luma3DS recommended)
- WiFi connection to your local network
- Companion server running on PC/NAS (Python 3.6+)

## 🔨 Building

### Install devkitPro (macOS)
```bash
# Install via pacman
brew install devkitpro-pacman
sudo dkp-pacman -S 3ds-dev

# Set environment
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export PATH=$DEVKITARM/bin:$PATH
```

### Build .3dsx (Homebrew)
```bash
cd 3Ds-CloudSaver
make
# Output: CloudSaver.3dsx
```

### Build .cia (Installable)
```bash
# First, generate the icon and banner:
# Place a 48x48 PNG as meta/icon.png
# Then generate the required files:
bannertool makesmdh -s "3DS CloudSaver" -l "Cloud Save Manager" -p "Jxstn" -i meta/icon.png -o meta/icon.icn
bannertool makebanner -i meta/banner.png -a meta/audio.wav -o meta/banner.bnr

# Build the CIA:
make cia
# Output: cia/CloudSaver.cia
```

## 🖥️ Companion Server

The companion server runs on your PC or NAS and handles file storage.

### Setup
```bash
cd server
python3 server.py --port 5000 --dir ./cloudsaves
```

### Options
| Flag    | Default        | Description            |
|---------|----------------|------------------------|
| --port  | 5000           | TCP port to listen on  |
| --dir   | ./cloudsaves   | Storage directory      |
| --bind  | 0.0.0.0        | Bind address           |

### Docker (optional)
```bash
docker run -d \
  -p 5000:5000 \
  -v /path/to/saves:/data \
  --name cloudsaver-server \
  python:3.11-slim \
  python3 /app/server.py --dir /data
```

## 🎮 Usage

### First Launch
1. Install CloudSaver.cia on your 3DS (via FBI or similar)
2. Launch the app
3. Enter a name for your device (e.g., "My3DS")
4. Go to **Settings** → configure your server:
   - Host: Your PC's IP address (e.g., `192.168.1.100`)
   - Port: `5000`
   - Username/Password: (not needed for CSTP)

### Browsing Games
- The **top screen** shows all installed games as a grid of covers
- Use **D-Pad** or **Circle Pad** to navigate
- Press **A** to select a game and view its cloud saves

### Syncing Saves
- **Upload**: Select a game → Press **Y** → Enter commit message
- **Download**: Select a game → Select a save → Press **A** to import
- **Sync All**: Tap "Sync All" in the navigation bar → Enter commit message

### Navigation (Bottom Screen)
| Tab       | Description              |
|-----------|--------------------------|
| 🎮 Games  | Browse games & saves    |
| 🔄 Sync   | Sync all games at once  |
| ⚙️ Settings| Server, language, device |

### Controls
| Button  | Action                        |
|---------|-------------------------------|
| A       | Select / Confirm / Download   |
| B       | Back / Cancel                 |
| X       | Edit save description         |
| Y       | Upload current save           |
| START   | Exit app                      |
| D-Pad   | Navigate games/saves          |
| Touch   | Navigation bar / Settings     |

## 🌍 Supported Languages

| Code | Language   |
|------|------------|
| EN   | English    |
| DE   | Deutsch    |
| FR   | Français   |
| ES   | Español    |
| IT   | Italiano   |
| JA   | 日本語      |

## 📁 Project Structure

```
3Ds-CloudSaver/
├── Makefile                 # Build system
├── README.md
├── source/
│   ├── main.c               # Entry point & main loop
│   ├── config.c             # Configuration persistence
│   ├── lang.c               # Multi-language string tables
│   ├── ui.c                 # UI rendering (citro2d)
│   ├── save.c               # Save data extraction/import
│   ├── network.c            # CSTP network protocol
│   └── sync.c               # Sync engine
├── include/
│   ├── common.h             # Shared types & constants
│   ├── config.h             # Config API
│   ├── lang.h               # Localization API
│   ├── ui.h                 # UI API
│   ├── save.h               # Save manager API
│   ├── network.h            # Network API
│   └── sync.h               # Sync engine API
├── meta/
│   ├── app.rsf              # CIA metadata
│   └── icon.png             # App icon (48x48)
├── romfs/
│   └── lang/                # (reserved for future lang files)
├── gfx/                     # Graphics assets
├── cia/                     # Built CIA output
├── server/
│   └── server.py            # Companion server (Python)
└── build/                   # Build artifacts (generated)
```

## 🔧 Protocol: CSTP (CloudSaver Transfer Protocol)

A simple TCP protocol for file operations:

```
Request:  [4B command][4B payload_length][payload...]
Response: [4B status ][4B payload_length][payload...]
```

| Command | Description                          |
|---------|--------------------------------------|
| PING    | Test connection → PONG               |
| LIST    | List directory → newline-sep names   |
| MKDR    | Create directory (recursive)         |
| UPLD    | Upload file (path + data)            |
| DNLD    | Download file → file data            |
| DELE    | Delete file or directory             |
| EXST    | Check if path exists → YES/NO        |

## 📜 License

MIT License – See [LICENSE](LICENSE) for details.

## 🙏 Credits

- [devkitPro](https://devkitpro.org/) – ARM toolchain for 3DS
- [libctru](https://github.com/devkitPro/libctru) – 3DS system library
- [citro2d](https://github.com/devkitPro/citro2d) – 2D rendering
- Color scheme inspired by [Catppuccin](https://catppuccin.com/)
