# TikTokWar

Clean C++ + SFML 2.5.1 project skeleton for a TikTok-style tower battle game.

The game keeps gameplay logic independent from TikTok. External integrations can send generic local JSON messages to the game; TikTok-specific rule mapping stays in the listener / bridge.

## Requirements
- CMake
- C++17 compiler
- SFML 2.5.x

Linux/WSL:
- g++ with C++17 support
- SFML (`sudo apt install libsfml-dev`)

Windows:
- Visual Studio Build Tools or Visual Studio with the C++ workload
- CMake available in `PATH`
- SFML 2.5.x/2.6.x for the same compiler architecture as your build, for example Visual C++ 17 2022 64-bit

## Build
Linux/WSL:
```bash
./build.sh
```

Run after build:
```bash
./build.sh --run 4
```

Or run manually from the build directory:
```bash
cd build
./tiktok_war 4
```

Windows:
```bat
build_windows.bat --sfml-dir C:\SFML-2.6.1\lib\cmake\SFML
```

Run after build:
```bat
build_windows.bat --run 4 --sfml-dir C:\SFML-2.6.1\lib\cmake\SFML
```

You can also set `SFML_ROOT=C:\SFML-2.6.1`; the script uses it to copy SFML DLLs from `%SFML_ROOT%\bin` next to `tiktok_war.exe`.

## Runtime options
```bash
./tiktok_war [N] [--input-host HOST] [--input-port PORT] [--disable-network-input]
```

- `N`: tower count, between `2` and `4`. Default: `3`.
- `--input-host`: local UDP bind address. Default: `127.0.0.1`.
- `--input-port`: local UDP port. Default: `7000`.
- `--disable-network-input`: disables the local JSON input listener.

Example:
```bash
./tiktok_war 4 --input-port 7000
```

## Keyboard controls
Keyboard input still works in parallel with network input.

For 4 towers, each row controls one source tower:

- Red row: `1`, `2`, `3`, `4`
- Blue row: `Q`, `W`, `E`, `R`
- Purple row: `A`, `S`, `D`, `F`
- Green row: `Z`, `X`, `C`, `V`

Within each row, the attack keys target the other towers in order and the last slot heals the source tower.

Global keys:
- `F5`: reset round
- `L`: toggle autoplay
- `Esc`: quit

## Local JSON input
The game listens for UDP JSON messages on:

```text
127.0.0.1:7000
```

The networking thread receives and parses JSON, then pushes normalized commands into a thread-safe queue. The game loop consumes that queue and applies commands through the existing gameplay systems. Gameplay is not executed directly from the networking thread.

Expected message format:
```json
{
  "message_type": "game_event",
  "action": "spawn Red Blue",
  "event": {
    "id": "evt_123",
    "type": "gift",
    "timestamp": "2026-06-10T20:00:00Z"
  },
  "user": {
    "id": "123",
    "username": "viewer1",
    "display_name": "Viewer One"
  },
  "payload": {
    "gift_name": "Rose",
    "coin_value": 1,
    "repeat_count": 1,
    "is_streak_final": true
  }
}
```

Supported `action` formats:

```text
spawn <SourceTeam> <TargetTeam>
heal <Team>
reset
```

Supported team names:

```text
Red, Blue, Purple, Green
```

`payload.repeat_count` is used as the command count. Invalid JSON, unsupported message types, invalid teams, and commands that reference towers not present in the current run are ignored with a warning instead of crashing the game.

## Test without TikTok
Start the game with 4 towers:

```bash
./build.sh --run 4
```

From another terminal, send a spawn command:

```bash
python3 tools/send_game_event.py spawn Red Blue --repeat 3
```

Send a heal command:

```bash
python3 tools/send_game_event.py heal Red --repeat 5
```

Use another port:

```bash
cd build
./tiktok_war 4 --input-port 7100
```

Then:

```bash
python3 tools/send_game_event.py spawn Blue Green --port 7100
```

## Architecture
- `model/` pure game data
- `systems/` gameplay logic
- `render/` drawing only
- `input/` command generation
  - `KeyboardAdapter`: SFML keyboard events to `GameCommand`
  - `TikTokAdapter`: generic local UDP JSON input to `GameCommand`
  - `InputManager`: aggregates input providers
- `core/` game loop orchestration
