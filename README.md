# Clawbrew
### A PS2 Homebrew Game — Decided by the Community

A homebrew PlayStation 2 game built from scratch in C. Runs on real PS2 hardware.

## What's New (v0.2)

- Collectibles: grab all items to advance to the next level
- Enemies: bouncing hazards that end your run if they touch you
- Lives system: 3 lives, reset on enemy contact
- Levels: difficulty scales as you progress (faster enemies)
- HUD: score, lives, and level displayed on screen
- Custom pixel font for numbers
- Dark blue playfield with border

## Current State

Full game loop with:
- Player movement via D-pad
- gsKit hardware-accelerated rendering
- Double-buffered 640x448 display
- Collectible items (yellow, cyan, orange diamonds)
- Bouncing enemies (red and purple)
- Score tracking, lives, and level progression
- Start button to quit

The community decides what this becomes next.

## Requirements

- [ps2sdk](https://github.com/ps2dev/ps2sdk) toolchain
- FreeMCBoot memory card
- USB drive or network loading

## Build

```bash
make
```

## Run on PS2

1. Copy `clawbrew.elf` to USB drive
2. Boot PS2 with FreeMCBoot
3. Launch uLaunchELF
4. Run the ELF from USB

## Run on Emulator

```bash
# PCSX2 or ps2client
make run
```

## Project Structure

```
src/main.c      - Game loop, rendering, and game logic
include/game.h  - Shared definitions and structures
Makefile        - Build configuration
```

## Contributing

All skill levels welcome. Open an issue or PR on GitHub. Even if you've never touched PS2 dev before, the codebase is small enough to learn from.

Ideas for next features:
- Sound effects via libsd
- Sprite graphics instead of colored squares
- A title screen
- High score saving to memory card
- Multiplayer support

## Links

- GitHub: https://github.com/coreylad/Clawbrew
