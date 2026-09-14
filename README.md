# FactoMan 🕹️

**FactoMan** is a clean, dark-mode desktop GUI client written in **C++ and FLTK** (Fast Light Tool Kit) for managing headless Factorio servers hosted on [factorio.zone](https://factorio.zone).

---

## Features

- **Modern Native Dark Mode GUI**:
  - Clean native FLTK layout designed for clarity and responsiveness.
  - Live Factorio server stdout terminal display with auto-scroll.
  - Direct console command bar (e.g. `/players`, `/save`, `/time`).
- **Automated Queue Management**:
  - Automatically handles factorio.zone wait queues (`statusCode: 202`).
  - Calculates `(turnTime - currentTime) / 1000` seconds, displays a real-time countdown, and automatically resends the start request with `&turnTime=<turnTime>` when your turn arrives!
  - Cancel queue anytime by clicking "Stop Server".
- **Multi-User Sync & Token Directory (Supabase)**:
  - Keeps 4-5 friends synchronized with server status and IP.
  - **User Token Lookup**: Store user tokens by nickname/key in Supabase (`user_tokens` table, e.g. `"bablu"` -> `"TOKEN"`).
  - One-click **`[ Fetch Token ]`** button in Settings to automatically retrieve and populate the token.
- **100% Standalone Single-File Binary (Zero DLLs)**:
  - Statically embeds FLTK, libcurl, OpenSSL, and the C++ runtime.
  - Just send `factoman.exe` (1 single file) to your friends—no missing DLL errors!

---

## Quick Start (Windows)

Run the single standalone executable:
```powershell
.\build\factoman.exe
```

---

## Supabase Setup (Sync & Token Directory)

1. Open your Supabase Dashboard -> **SQL Editor**.
2. Run the script in [`supabase_schema.sql`](supabase_schema.sql).
3. To add tokens for your friends:
   ```sql
   INSERT INTO public.user_tokens (key, token)
   VALUES ('bablu', 'TOKEN');
   ```
4. In FactoMan Settings, type `bablu` and click **Fetch Token** to automatically pull the token!
