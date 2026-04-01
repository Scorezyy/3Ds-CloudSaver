<p align="center"># 3DS CloudSaver ☁️🎮

  <img src="meta/icon.png" width="96" height="96" alt="CloudSaver Icon">

</p>A cloud save manager for the Nintendo 3DS. Synchronize your save files across multiple 3DS consoles using a companion server.



<h1 align="center">3DS CloudSaver</h1>![Platform](https://img.shields.io/badge/platform-Nintendo%203DS-red)

![License](https://img.shields.io/badge/license-MIT-blue)

<p align="center">![Language](https://img.shields.io/badge/language-C-blue)

  <b>☁️ Cloud Save Manager for Nintendo 3DS</b><br>

  Sync your save files across multiple consoles — no PC needed in between.## ✨ Features

</p>

- **Cloud Save Sync** – Upload and download save files for any installed game

<p align="center">- **Multi-Device Support** – Share saves between multiple 3DS consoles

  <img src="https://img.shields.io/badge/platform-Nintendo%203DS-D32F2F?style=flat-square&logo=nintendo3ds&logoColor=white" alt="Platform">- **Sync All** – One-tap sync for all games with commit messages

  <img src="https://img.shields.io/badge/language-C-555555?style=flat-square&logo=c&logoColor=white" alt="Language">- **Game Browser** – Beautiful top-screen grid showing all installed game covers

  <img src="https://img.shields.io/badge/license-MIT-blue?style=flat-square" alt="License">- **Save Details** – View creation date, device name, region, and custom descriptions

  <img src="https://img.shields.io/github/v/release/Scorezyy/3Ds-CloudSaver?style=flat-square&color=brightgreen" alt="Release">- **Multi-Language** – English, Deutsch, Français, Español, Italiano, 日本語

</p>- **Multi-Region** – EUR, USA, JPN, KOR support

- **Persistent Config** – Device name, server settings, and preferences saved locally

---- **Auto-Connect** – Automatically connects to your server on app launch



## 📖 Table of Contents## 🗂️ Cloud Storage Structure



- [What is this?](#-what-is-this)```

- [Features](#-features)/DEVICENAME/GAMENAME/GAMEID/SAVEFILES/

- [Download & Install](#-download--install)├── MyDevice_20260401_120000.sav

- [Server Setup](#%EF%B8%8F-server-setup)├── MyDevice_20260401_120000.meta

- [How to Use](#-how-to-use)├── OtherDevice_20260330_180000.sav

- [Controls](#-controls)└── OtherDevice_20260330_180000.meta

- [Build from Source](#-build-from-source)```

- [Protocol](#-protocol-cstp)

- [License & Credits](#-license--credits)## 📋 Requirements

- [About this Project](#-about-this-project)

### Build Requirements

---- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM

- [libctru](https://github.com/devkitPro/libctru)

## 💡 What is this?- [citro2d](https://github.com/devkitPro/citro2d) & citro3d

- [makerom](https://github.com/3DSGuy/Project_CTR/releases) (for CIA building)

**3DS CloudSaver** lets you back up your game saves to a server running on your PC or NAS.  - [bannertool](https://github.com/Steveice10/bannertool/releases) (for CIA building)

You can upload saves from one 3DS and download them on another — like cloud saves, but for the 3DS.

### Runtime Requirements

**You need two things:**- Nintendo 3DS with CFW (Luma3DS recommended)

1. A 3DS with Custom Firmware (e.g. [Luma3DS](https://github.com/LumaTeam/Luma3DS))- WiFi connection to your local network

2. A computer on the same WiFi network running the companion server- Companion server running on PC/NAS (Python 3.6+)



---## 🔨 Building



## ✨ Features### Install devkitPro (macOS)

```bash

| Feature | Description |# Install via pacman

|---------|-------------|brew install devkitpro-pacman

| ☁️ **Cloud Sync** | Upload & download saves for any installed game |sudo dkp-pacman -S 3ds-dev

| 🔄 **Multi-Device** | Share saves between multiple 3DS consoles |

| 📦 **Sync All** | Backup all games at once with one tap |# Set environment

| 🎮 **Game Browser** | Grid view of all installed games with icons |export DEVKITPRO=/opt/devkitpro

| 🏷️ **Save Versioning** | Every backup has a timestamp, device name & description |export DEVKITARM=$DEVKITPRO/devkitARM

| 🌍 **6 Languages** | EN · DE · FR · ES · IT · JA |export PATH=$DEVKITARM/bin:$PATH

| 💾 **Persistent Config** | Settings are saved locally on the 3DS |```



---### Build .3dsx (Homebrew)

```bash

## 📥 Download & Installcd 3Ds-CloudSaver

make

Go to the [**Releases**](https://github.com/Scorezyy/3Ds-CloudSaver/releases/latest) page and download the file you need:# Output: CloudSaver.3dsx

```

| File | Format | How to install |

|------|--------|---------------|### Build .cia (Installable)

| `CloudSaver.3dsx` | Homebrew | Copy to `sd:/3ds/` → open with Homebrew Launcher |```bash

| `CloudSaver.cia` | Installable | Copy to SD card → install with [FBI](https://github.com/Steveice10/FBI/releases) |# First, generate the icon and banner:

# Place a 48x48 PNG as meta/icon.png

> **Tip:** The `.cia` version installs as a regular app on your Home Menu. The `.3dsx` version runs from the Homebrew Launcher.# Then generate the required files:

bannertool makesmdh -s "3DS CloudSaver" -l "Cloud Save Manager" -p "Jxstn" -i meta/icon.png -o meta/icon.icn

---bannertool makebanner -i meta/banner.png -a meta/audio.wav -o meta/banner.bnr



## 🖥️ Server Setup# Build the CIA:

make cia

The server is a small Python script that runs on your **PC, Mac, or NAS**.  # Output: cia/CloudSaver.cia

Your 3DS sends saves to this server over your local WiFi.```



### Option 1 — Python (simplest)## 🖥️ Companion Server



> Requires: [Python 3.6+](https://www.python.org/downloads/)The companion server runs on your PC or NAS and handles file storage.



```bash### Setup

cd server```bash

python3 server.py --port 5000cd server

```python3 server.py --port 5000 --dir ./cloudsaves

```

That's it. The server is running. Saves will be stored in `./cloudsaves/`.

### Options

<details>| Flag    | Default        | Description            |

<summary>📋 All server options</summary>|---------|----------------|------------------------|

| --port  | 5000           | TCP port to listen on  |

| Flag | Default | Description || --dir   | ./cloudsaves   | Storage directory      |

|------|---------|-------------|| --bind  | 0.0.0.0        | Bind address           |

| `--port` | `5000` | Port to listen on |

| `--dir` | `./cloudsaves` | Where saves are stored |### Docker (optional)

| `--bind` | `0.0.0.0` | Network interface to bind to |```bash

docker run -d \

```bash  -p 5000:5000 \

# Example with custom settings  -v /path/to/saves:/data \

python3 server.py --port 8080 --dir /home/user/3ds-saves  --name cloudsaver-server \

```  python:3.11-slim \

  python3 /app/server.py --dir /data

</details>```



### Option 2 — Docker## 🎮 Usage



```bash### First Launch

cd server1. Install CloudSaver.cia on your 3DS (via FBI or similar)

docker compose up -d2. Launch the app

```3. Enter a name for your device (e.g., "My3DS")

4. Go to **Settings** → configure your server:

<details>   - Host: Your PC's IP address (e.g., `192.168.1.100`)

<summary>📋 docker-compose.yml details</summary>   - Port: `5000`

   - Username/Password: (not needed for CSTP)

```yaml

version: "3.8"### Browsing Games

services:- The **top screen** shows all installed games as a grid of covers

  cloudsaver-server:- Use **D-Pad** or **Circle Pad** to navigate

    build: .- Press **A** to select a game and view its cloud saves

    container_name: cloudsaver-server

    ports:### Syncing Saves

      - "5000:5000"- **Upload**: Select a game → Press **Y** → Enter commit message

    volumes:- **Download**: Select a game → Select a save → Press **A** to import

      - ./cloudsaves:/data- **Sync All**: Tap "Sync All" in the navigation bar → Enter commit message

    restart: unless-stopped

```### Navigation (Bottom Screen)

| Tab       | Description              |

</details>|-----------|--------------------------|

| 🎮 Games  | Browse games & saves    |

### 🔍 Find your Server IP| 🔄 Sync   | Sync all games at once  |

| ⚙️ Settings| Server, language, device |

You'll need your computer's local IP address to connect from the 3DS.

### Controls

| OS | Command || Button  | Action                        |

|----|---------||---------|-------------------------------|

| **Windows** | `ipconfig` → look for `IPv4 Address` || A       | Select / Confirm / Download   |

| **macOS** | `ipconfig getifaddr en0` || B       | Back / Cancel                 |

| **Linux** | `hostname -I` || X       | Edit save description         |

| Y       | Upload current save           |

It will look something like `192.168.1.100`.| START   | Exit app                      |

| D-Pad   | Navigate games/saves          |

---| Touch   | Navigation bar / Settings     |



## 🎮 How to Use## 🌍 Supported Languages



### First Launch| Code | Language   |

|------|------------|

1. **Open CloudSaver** on your 3DS| EN   | English    |

2. **Enter a device name** (e.g. "My3DS") — this identifies your console| DE   | Deutsch    |

3. **Go to Settings** and enter:| FR   | Français   |

   - **Host** — your server's IP address (e.g. `192.168.1.100`)| ES   | Español    |

   - **Port** — `5000` (or whatever you set)| IT   | Italiano   |

4. Done! The app will auto-connect on every launch| JA   | 日本語      |



### Uploading a Save## 📁 Project Structure



1. Browse your games in the **Games** tab```

2. Select a game → press **Y**3Ds-CloudSaver/

3. Enter a description (e.g. "Before Elite Four")├── Makefile                 # Build system

4. Your save is now in the cloud ☁️├── README.md

├── source/

### Downloading a Save│   ├── main.c               # Entry point & main loop

│   ├── config.c             # Configuration persistence

1. Select a game → browse its cloud saves│   ├── lang.c               # Multi-language string tables

2. Select the save you want → press **A**│   ├── ui.c                 # UI rendering (citro2d)

3. The save is imported back to your 3DS│   ├── save.c               # Save data extraction/import

│   ├── network.c            # CSTP network protocol

### Sync All Games│   └── sync.c               # Sync engine

├── include/

1. Go to the **Sync** tab│   ├── common.h             # Shared types & constants

2. Press **Sync All**│   ├── config.h             # Config API

3. All games are backed up at once│   ├── lang.h               # Localization API

│   ├── ui.h                 # UI API

---│   ├── save.h               # Save manager API

│   ├── network.h            # Network API

## 🎯 Controls│   └── sync.h               # Sync engine API

├── meta/

| Button | Action |│   ├── app.rsf              # CIA metadata

|--------|--------|│   └── icon.png             # App icon (48x48)

| **A** | Select / Confirm / Download save |├── romfs/

| **B** | Back / Cancel |│   └── lang/                # (reserved for future lang files)

| **X** | Edit save description |├── gfx/                     # Graphics assets

| **Y** | Upload current save |├── cia/                     # Built CIA output

| **D-Pad** | Navigate |├── server/

| **Touch** | Navigation tabs & settings |│   └── server.py            # Companion server (Python)

| **START** | Exit |└── build/                   # Build artifacts (generated)

```

---

## 🔧 Protocol: CSTP (CloudSaver Transfer Protocol)

## 🔨 Build from Source

A simple TCP protocol for file operations:

<details>

<summary>Click to expand build instructions</summary>```

Request:  [4B command][4B payload_length][payload...]

### PrerequisitesResponse: [4B status ][4B payload_length][payload...]

```

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM

- libctru, citro2d, citro3d — install via `dkp-pacman -S 3ds-dev`| Command | Description                          |

- [makerom](https://github.com/3DSGuy/Project_CTR/releases) + [bannertool](https://github.com/Steveice10/bannertool/releases) (for CIA only)|---------|--------------------------------------|

| PING    | Test connection → PONG               |

### Build .3dsx| LIST    | List directory → newline-sep names   |

| MKDR    | Create directory (recursive)         |

```bash| UPLD    | Upload file (path + data)            |

make| DNLD    | Download file → file data            |

```| DELE    | Delete file or directory             |

| EXST    | Check if path exists → YES/NO        |

### Build .cia

## 📜 License

```bash

make ciaMIT License – See [LICENSE](LICENSE) for details.

```

## 🙏 Credits

Output goes to `cia/CloudSaver.cia`.

- [devkitPro](https://devkitpro.org/) – ARM toolchain for 3DS

</details>- [libctru](https://github.com/devkitPro/libctru) – 3DS system library

- [citro2d](https://github.com/devkitPro/citro2d) – 2D rendering

---- Color scheme inspired by [Catppuccin](https://catppuccin.com/)


## 🔧 Protocol (CSTP)

<details>
<summary>Click to expand protocol details</summary>

CloudSaver uses **CSTP** (CloudSaver Transfer Protocol), a simple TCP protocol:

```
Request:  [4B command][4B payload_length][payload...]
Response: [4B status ][4B payload_length][payload...]
```

| Command | Description |
|---------|-------------|
| `PING` | Test connection → `PONG` |
| `LIST` | List directory contents |
| `MKDR` | Create directory (recursive) |
| `UPLD` | Upload file |
| `DNLD` | Download file |
| `DELE` | Delete file or directory |
| `EXST` | Check if path exists |

</details>

---

## 📜 License & Credits

**MIT License** — see [LICENSE](LICENSE)

Built with [devkitPro](https://devkitpro.org/) · [libctru](https://github.com/devkitPro/libctru) · [citro2d](https://github.com/devkitPro/citro2d)  
UI colors inspired by [Catppuccin](https://catppuccin.com/) 🎨

---

## 💬 About this Project

> This is my very first Nintendo 3DS application — and honestly, my first real project in C.  
> I had zero experience with C, no idea how 3DS homebrew development worked, and just… started.  
> Learned everything along the way: low-level file I/O, socket programming, GPU rendering, the whole thing.  
>  
> If the code isn't perfect — that's fine. The point was to build something real, learn by doing, and ship it.  
> And here it is. 🚀
>
> — **Jxstn**
