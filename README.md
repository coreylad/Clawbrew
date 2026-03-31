# Clawbrew
### A PS2 Homebrew Game -- Decided by the Community

A homebrew PlayStation 2 game built from scratch in C. Runs on real PS2 hardware and emulators.

## What's New (v0.3)

- **Title screen** with "CLAW" logo and high score display
- **Pause system** -- START button toggles pause overlay
- **Powerups**: Speed Boost (faster movement) and Shield (absorbs one hit)
- **Chaser enemies** appear on level 2+ -- slowly drift toward the player
- **Death flash** -- white screen burst on enemy contact
- **High score tracking** across runs within a session
- **Powerup timer bar** shows remaining effect duration
- **Improved enemy spawning** -- speed caps at level-appropriate values

## What Was Already There (v0.2)

- Collectibles: grab all items to advance to the next level
- Enemies: bouncing hazards that end your run if they touch you
- Lives system: 3 lives, reset on enemy contact
- Levels: difficulty scales as you progress (faster enemies)
- HUD: score, lives, and level displayed on screen
- Custom pixel font for numbers
- Dark blue playfield with border

## Controls

| Button | Action |
|--------|--------|
| X | Start game (title) / Return to title (game over) |
| D-Pad | Move player |
| START | Pause / Resume |

## Requirements

- [ps2sdk](https://github.com/ps2dev/ps2sdk) toolchain
- FreeMCBoot memory card (real hardware)
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
src/main.c      - Game loop, rendering, input, and game logic
include/game.h  - Shared definitions, structures, and prototypes
Makefile        - Build configuration for ps2sdk
```

## Contributing

All skill levels welcome. Open an issue or PR on GitHub. Even if you've never touched PS2 dev before, the codebase is small enough to learn from.

**Good first contributions:**
- Add a background music track (libsd)
- Replace colored squares with actual sprite graphics
- Add a high score save to memory card (libmc)
- Create a second level layout (platforms, walls)
- Add sound effects for collectibles and enemy hits

**For experienced PS2 devs:**
- Sprite rendering via gsKit textures
- Multiplayer support (second controller)
- Level editor data format
- Networked co-op via ps2client

The project is intentionally simple. Every file fits in your head. That's the point.

## Support This Project

If you want to support development, there's a **Sponsor** button on the GitHub repo. All funds go toward hardware testing, community bounties for features, and keeping the project alive.

GitHub: https://github.com/coreylad/Clawbrew

## License

Public domain. Do whatever you want with it.
