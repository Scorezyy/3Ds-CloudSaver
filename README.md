<p align="center">
  <img src="meta/icon.png" width="96" height="96" alt="CloudSaver Icon">
  <br><br>
  <b>3DS CloudSaver</b><br>
  <sub>☁️ Cloud Save Manager for Nintendo 3DS</sub><br>
  <sub>Sync your save files across multiple consoles.</sub>
  <br><br>
  <a href="https://github.com/Scorezyy/3Ds-CloudSaver/releases/latest"><img src="https://img.shields.io/github/v/release/Scorezyy/3Ds-CloudSaver?style=flat-square&color=brightgreen" alt="Release"></a>
  <img src="https://img.shields.io/badge/platform-Nintendo%203DS-D32F2F?style=flat-square" alt="Platform">
  <img src="https://img.shields.io/badge/language-C-555555?style=flat-square" alt="Language">
  <img src="https://img.shields.io/badge/license-MIT-blue?style=flat-square" alt="License">
</p>

---

## 📖 Table of Contents

- [What is this?](#-what-is-this)
- [Features](#-features)
- [Download \& Install](#-download--install)
- [Server Setup](#%EF%B8%8F-server-setup)
- [How to Use](#-how-to-use)
- [Controls](#-controls)
- [Build from Source](#-build-from-source)
- [Protocol](#-protocol-cstp)
- [License \& Credits](#-license--credits)
- [About this Project](#-about-this-project)

---

## 💡 What is this?

**3DS CloudSaver** lets you back up your game saves to a server running on your PC or NAS.
Upload saves from one 3DS, download them on another — like cloud saves, but for the 3DS.

**You need two things:**

1. A 3DS with Custom Firmware (e.g. [Luma3DS](https://github.com/LumaTeam/Luma3DS))
2. A computer on the same WiFi running the companion server

---

## ✨ Features

| | Feature | Description |
|---|---------|-------------|
| ☁️ | **Cloud Sync** | Upload and download saves for any installed game |
| 🔄 | **Multi-Device** | Share saves between multiple 3DS consoles |
| 📦 | **Sync All** | Backup all games at once with one tap |
| 🎮 | **Game Browser** | Grid view of all your installed games with icons |
| 🏷️ | **Save Versioning** | Every backup has a timestamp, device name, and description |
| 🌍 | **6 Languages** | EN · DE · FR · ES · IT · JA |
| 💾 | **Persistent Config** | Your settings are saved locally on the 3DS |

---

## 📥 Download & Install

Head to the [**Releases**](https://github.com/Scorezyy/3Ds-CloudSaver/releases/latest) page and grab the file you need:

| File | Format | How to install |
|------|--------|----------------|
| `CloudSaver.3dsx` | Homebrew | Copy to `sd:/3ds/` → open via Homebrew Launcher |
| `CloudSaver.cia` | Installable | Copy to SD → install with [FBI](https://github.com/Steveice10/FBI/releases) |

> [!TIP]
> The `.cia` installs as a regular app on your Home Menu.
> The `.3dsx` runs from the Homebrew Launcher.

---

## 🖥️ Server Setup

The server is a small Python script that runs on your **PC, Mac, or NAS**.
Your 3DS sends saves to it over your local WiFi.

### Option 1 — Python (simplest)

> **Requires:** [Python 3.6+](https://www.python.org/downloads/)

```bash
cd server
python3 server.py --port 5000
```

That's it. Saves are stored in `./cloudsaves/`.

<details>
<summary><b>All server options</b></summary>
<br>

| Flag | Default | Description |
|------|---------|-------------|
| `--port` | `5000` | Port to listen on |
| `--dir` | `./cloudsaves` | Where saves are stored |
| `--bind` | `0.0.0.0` | Network interface to bind to |

Example:
```bash
python3 server.py --port 8080 --dir /home/user/3ds-saves
```

</details>

### Option 2 — Docker

```bash
cd server
docker compose up -d
```

<details>
<summary><b>docker-compose.yml</b></summary>
<br>

```yaml
version: "3.8"
services:
  cloudsaver-server:
    build: .
    container_name: cloudsaver-server
    ports:
      - "5000:5000"
    volumes:
      - ./cloudsaves:/data
    restart: unless-stopped
```

</details>

### Find your Server IP

You need your computer's local IP to connect from the 3DS:

| OS | Command |
|----|---------|
| Windows | `ipconfig` → look for **IPv4 Address** |
| macOS | `ipconfig getifaddr en0` |
| Linux | `hostname -I` |

It will look something like `192.168.1.100`.

---

## 🎮 How to Use

### First Launch

1. Open **CloudSaver** on your 3DS
2. Enter a **device name** (e.g. `My3DS`) — this identifies your console
3. Go to **Settings** and enter your server info:
   - **Host** → your PC's IP (e.g. `192.168.1.100`)
   - **Port** → `5000`
4. Done — the app auto-connects on every launch

### Upload a Save

1. Go to **Games** tab → select a game
2. Press **Y** → enter a description (e.g. `Before Elite Four`)
3. Save is uploaded ☁️

### Download a Save

1. Select a game → browse its cloud saves
2. Pick the one you want → press **A**
3. Save is imported to your 3DS

### Sync Everything

1. Go to **Sync** tab → press **Sync All**
2. All games backed up at once

---

## 🎯 Controls

| Button | Action |
|--------|--------|
| **A** | Select · Confirm · Download |
| **B** | Back · Cancel |
| **X** | Edit save description |
| **Y** | Upload save |
| **D-Pad** | Navigate |
| **Touch** | Tabs and settings |
| **START** | Exit |

---

## 🔨 Build from Source

<details>
<summary><b>Click to expand</b></summary>
<br>

**Prerequisites:**
- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM
- libctru, citro2d, citro3d → `dkp-pacman -S 3ds-dev`
- [makerom](https://github.com/3DSGuy/Project_CTR/releases) + [bannertool](https://github.com/Steveice10/bannertool/releases) (CIA only)

**Build .3dsx:**
```bash
make
```

**Build .cia:**
```bash
make cia
```

</details>

---

## 🔧 Protocol (CSTP)

<details>
<summary><b>Click to expand</b></summary>
<br>

CloudSaver uses **CSTP** (CloudSaver Transfer Protocol) — a simple TCP protocol:

```
Request:  [4B command][4B payload_length][payload...]
Response: [4B status ][4B payload_length][payload...]
```

| Command | Description |
|---------|-------------|
| `PING` | Connection test → `PONG` |
| `LIST` | List directory contents |
| `MKDR` | Create directory (recursive) |
| `UPLD` | Upload file |
| `DNLD` | Download file |
| `DELE` | Delete file or directory |
| `EXST` | Check if path exists |

</details>

---

## 📜 License & Credits

MIT License — see [LICENSE](LICENSE)

Built with [devkitPro](https://devkitpro.org/) · [libctru](https://github.com/devkitPro/libctru) · [citro2d](https://github.com/devkitPro/citro2d)
UI colors inspired by [Catppuccin](https://catppuccin.com/) 🎨

---

## 💬 About this Project

> This is my very first Nintendo 3DS application — and my first real project in C.
> I had zero experience with C, no idea how 3DS homebrew development worked, and just… started.
> Learned everything along the way: low-level file I/O, socket programming, GPU rendering, the whole thing.
>
> If the code isn't perfect — that's fine. The point was to build something real, learn by doing, and ship it.
> And here it is. 🚀
>
> — **Jxstn**
