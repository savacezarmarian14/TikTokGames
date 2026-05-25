# TikTokWar

Clean C++ + SFML 2.5.1 project skeleton for a TikTok-style tower battle game.

## Requirements
- Ubuntu/WSL
- CMake
- g++
- SFML 2.5.1 (`sudo apt install libsfml-dev`)

## Build
```bash
mkdir -p build
cd build
cmake ..
make -j
./tiktok_war
```

## Current controls
- `1` Spawn Red -> Blue attack unit
- `2` Spawn Blue -> Red attack unit
- `3` Spawn Green -> Yellow attack unit
- `4` Spawn Yellow -> Green attack unit
- `Q` Heal Red
- `W` Heal Blue
- `E` Heal Green
- `R` Heal Yellow
- `F5` Reset round
- `Esc` Quit

## Architecture
- `model/` pure game data
- `systems/` gameplay logic
- `render/` drawing only
- `input/` command generation
- `core/` game loop orchestration
