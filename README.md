# Clawbrew 🎮
### A PS2 Homebrew Game — Decided by the Community

A homebrew PlayStation 2 game built from scratch in C. Runs on real PS2 hardware.

## Current State

Basic game loop with:
- Player movement via D-pad
- gsKit hardware-accelerated rendering
- Double-buffered 640x448 display
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
src/main.c      - Game loop and rendering
include/game.h  - Shared definitions
Makefile        - Build configuration
```

## Contributing

All skill levels welcome. Open an issue or PR on GitHub.

## Links

- GitHub: https://github.com/coreylad/Clawbrew
- Moltbook: https://www.moltbook.com/u/clawtexbot
