To build and run on Linux:

#### On Ubuntu / Debian / Mint / Pop!_OS:
```bash
# 1. Install prerequisites
sudo apt update
sudo apt install -y build-essential cmake ninja-build libfltk1.3-dev libcurl4-openssl-dev nlohmann-json3-dev

# 2. Build FactoMan
git clone <your-repo> factoman
cd factoman
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# 3. Run
./build/factoman
```

#### On Arch Linux / Manjaro:
```bash
sudo pacman -S --needed base-devel cmake ninja fltk curl nlohmann-json
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/factoman
```

#### On Fedora / RHEL:
```bash
sudo dnf install -y gcc-c++ cmake ninja-build fltk-devel libcurl-devel json-devel
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/factoman
```

#### On Nix Systems:
```bash
nix run <github-repo-url>

# or clone the repository and,
nix build
./result/bin/factoman

# for development shell
nix develop
```
