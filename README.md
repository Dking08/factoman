# FactoMan 🕹️

**FactoMan** is a clean, dark-mode desktop GUI client written in **C++ and FLTK** (Fast Light Tool Kit) for managing headless Factorio servers hosted on [factorio.zone](https://factorio.zone).

It gives you and your friends real-time telemetry, live streaming server logs, remote console command input, and seamless multi-user state synchronization via **Supabase**.

---

## Features

- **Modern Native Dark Mode GUI**:
  - Clean native FLTK layout designed for readability.
  - Server Status readout & active IP:Port field with a 1-click **"Copy IP"** button.
  - Live Factorio server stdout terminal display with auto-scroll.
  - Direct console command bar (e.g. `/players`, `/save`, `/time`).
- **Factorio.zone Integration**:
  - Connects to `wss://factorio.zone/ws` to receive real-time visit secrets and telemetry with automated 15-second keepalive.
  - Authenticates with your persistent `userToken` via `/api/user/login`.
  - Automatically captures allocated IP and port (`type == "running"`, `socket`, and `launchId`).
  - Starts and stops server instances via `/api/instance/start` and `/api/instance/stop`.
  - Dispatches remote server console commands via `/api/instance/console`.
- **Multi-User Sync (Supabase Free Tier)**:
  - Keeps 4-5 friends synchronized in real-time.
  - Sync updates cleanly display in the bottom status bar without polluting the Factorio server console log.
  - Polling interval set to 6 seconds with on-change detection.
- **100% Standalone Single-File Binary (Zero DLLs)**:
  - Statically embeds FLTK, libcurl, OpenSSL, and the C++ runtime.
  - Just copy `factoman.exe` (1 single file) to your friends—no missing DLL errors!

---

## Quick Start (Running on Windows)

Simply run the single standalone executable:
```powershell
.\build\factoman.exe
```
*(Or double-click `factoman.exe` in Windows Explorer. You do NOT need any loose `.dll` files.)*

---

## Building on Linux

FactoMan is 100% standard C++17 and FLTK, and builds natively on Linux:

### Ubuntu / Debian / Mint
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build libfltk1.3-dev libcurl4-openssl-dev nlohmann-json3-dev
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/factoman
```

### Arch Linux / Manjaro
```bash
sudo pacman -S --needed base-devel cmake ninja fltk curl nlohmann-json
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/factoman
```

### Fedora / RHEL
```bash
sudo dnf install -y gcc-c++ cmake ninja-build fltk-devel libcurl-devel json-devel
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/factoman
```

---

## Multi-User Sync Setup (Supabase)

1. In Supabase Dashboard -> **SQL Editor**, run the script in [`supabase_schema.sql`](supabase_schema.sql).
2. In Supabase -> **Project Settings -> API**, copy your **Project URL** and the key labeled **`anon` `public`**.
3. In FactoMan, press `Ctrl+S` (or click **Settings...**), paste your Supabase URL & Anon Key, and set your nickname.
4. When any friend clicks "Start Server", all clients update within seconds!
