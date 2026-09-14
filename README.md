# FactoMan 🕹️

**FactoMan** is a classic, retro-styled desktop GUI client written in **C++ and FLTK** (Fast Light Tool Kit) for managing headless Factorio servers hosted on [factorio.zone](https://factorio.zone).

Designed with an authentic 90s UNIX/Motif/Win95 aesthetic, FactoMan gives you and your friends real-time telemetry, live streaming server logs, remote console command input, and seamless multi-user state synchronization via **Supabase**.

---

## Features

- **Classic Ancient GUI Aesthetic**:
  - Tactile 3D beveled panels (`FL_UP_BOX`, `FL_DOWN_BOX`).
  - Analog-style LED status lamp (Gray = Offline, Amber = Starting, Green = Running, Red = Stopping).
  - High-contrast digital readout for the server IP & Port with a 1-click **"Copy IP"** button.
  - CRT-style monochrome terminal display with bright phosphor-green monospace text (`#00FF66`) and auto-scrolling log buffer.
  - Direct console command bar (e.g. `/players`, `/save`, `/time`, `/alerts`).
- **Factorio.zone Integration**:
  - Connects to `wss://factorio.zone/ws` to receive real-time visit secrets and telemetry.
  - Authenticates with your persistent `userToken` via `/api/user/login`.
  - Automatically captures the allocated server IP and port from the WebSocket stream.
  - Starts and stops server instances via `/api/instance/start` and `/api/instance/stop`.
  - Dispatches remote server console commands via `/api/instance/console`.
- **Multi-User Sync (Supabase Free Tier)**:
  - Keeps 4-5 friends synchronized in real-time.
  - When friend **A** starts the server, friends **B, C, and D** instantly see the server status change to **RUNNING**, see the allocated IP automatically populate in their client, and see who initiated the action!
  - Cross-platform & lightweight: runs natively on Windows, Linux, and macOS.

---

## Directory Structure

```
factoman/
├── build/                      # Compiled standalone executable & runtime DLLs
│   ├── factoman.exe            # Main executable
│   ├── config.json             # App configuration
│   └── *.dll                   # Bundled runtime libraries (FLTK, Curl, OpenSSL, etc.)
├── src/
│   ├── ui/
│   │   ├── main_window.hpp/cpp     # FLTK Main Window & retro interface
│   │   └── settings_dialog.hpp/cpp # Configuration & Sync modal dialog
│   ├── config.hpp/cpp              # Configuration manager (JSON)
│   ├── http_client.hpp/cpp         # Libcurl HTTP wrapper
│   ├── factorio_zone_client.hpp/cpp# WebSocket listener & Factorio.zone REST client
│   ├── supabase_sync.hpp/cpp       # Supabase multi-user state synchronization
│   └── main.cpp                    # Application entry point
├── supabase_schema.sql         # SQL script to initialize Supabase sync table
├── CMakeLists.txt              # Cross-platform build script
└── config.json                 # Default configuration file
```

---

## Quick Start (Running the App)

### 1. Launching
You can run the pre-built portable binary directly:
```powershell
.\build\factoman.exe
```
Or double-click `factoman.exe` inside the `build/` folder.

### 2. Basic Server Management
1. When launched, FactoMan connects to `wss://factorio.zone/ws` and links your `userToken`.
2. Click **"START SERVER"** to boot the Factorio server.
3. The LED turns Amber (`STARTING...`), and the console streams server initialization logs.
4. Once the server allocates a network socket, the LED turns Green (`SERVER RUNNING`) and the IP field populates (e.g. `65.0.66.221:21914`).
5. Click **"Copy IP"** to copy the server address directly to your clipboard and share or paste into Factorio!
6. To issue commands, type in the **Command** field (e.g. `/players`) and press **Enter** or click **"Send"**.
7. To shutdown, click **"STOP SERVER"**.

---

## Setting Up Supabase Multi-User Sync (Optional but Recommended!)

To sync the server status and IP with your friends so anyone can start/stop and everyone gets the IP:

1. Create a free project at [supabase.com](https://supabase.com).
2. In your Supabase dashboard, go to the **SQL Editor** on the left menu.
3. Open [`supabase_schema.sql`](supabase_schema.sql), copy the contents, paste it into the Supabase SQL editor, and click **Run**.
4. In Supabase, go to **Project Settings** -> **API**:
   - Copy your **Project URL** (e.g. `https://xyzcompany.supabase.co`).
   - Copy your **Project API Key** (`anon` `public`).
5. In FactoMan, click **"Settings..."** (or `Ctrl+S`):
   - Paste the **Supabase URL**.
   - Paste the **Supabase Anon Key**.
   - Set your **Player Nickname** (e.g., `Alex`).
   - Click **Save**.
6. Share your `config.json` (or the URL and Anon key) with your friends. When any of you clicks "Start Server", all clients update within seconds!

---

## Building From Source

### Prerequisites
- **Compiler**: GCC / Clang / MSVC supporting C++17.
- **Build Tools**: CMake 3.20+, Ninja or Make.
- **Libraries**:
  - FLTK 1.4+
  - Libcurl with WebSocket support
  - OpenSSL 3.x
  - nlohmann/json 3.x

### On Windows (MSYS2 UCRT64)
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-fltk mingw-w64-ucrt-x86_64-curl mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### On Linux (Debian / Ubuntu)
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libfltk1.3-dev libcurl4-openssl-dev nlohmann-json3-dev
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### On macOS
```bash
brew install cmake ninja fltk curl nlohmann-json
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```
