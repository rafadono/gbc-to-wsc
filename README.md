# GBC to WonderSwan Color Compatibility Layer (HAL)

A generic, high-performance Hardware Abstraction Layer (HAL) and translation pipeline designed to port statically recompiled and decompiled Game Boy (GB) and Game Boy Color (GBC) games to the Bandai WonderSwan Color (WSC).

---

## Overview

Instead of running guest code dynamically on the WonderSwan Color's 3.072 MHz NEC V30MZ CPU, this project targets **ahead-of-time (AOT) static recompilation** and source decompilation into native C code (via `arcanite24/gb-recompiled` or community source decompilations). 

The `gbc_wsc_hal` bridges the recompiled and decompiled game logic directly to native WonderSwan Color hardware registers and memory.

```
+-------------------------------------------------------+
|    Recompiled / Decompiled Game Logic (C Source)      |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|         GBC to WSC COMPATIBILITY LAYER (HAL)          |
|  - Memory & Banking Bus ($0000 - $FFFF, MBC1, MBC5)   |
|  - PPU Translator (2BPP Bitplane Transpose, Palettes) |
|  - Sprites (8x8 & 8x16 OAM -> 128 WSC Sprites)        |
|  - Window Layer (GB Window -> WSC Screen 2)           |
|  - Input Polling ($FF00 Active-Low Key Matrix)        |
|  - Frame Pacing (4:5 Ratio: 59.7 Hz to 75.4 Hz)       |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|           WonderSwan Color Hardware / libws           |
+-------------------------------------------------------+
```

---

## Architectural Mapping

| Subsystem | Game Boy / Color (GBC) | WonderSwan Color (WSC) | Compatibility Handling |
| :--- | :--- | :--- | :--- |
| **RAM** | 32 KB WRAM + 16 KB VRAM (~48 KB) | 64 KB Unified IRAM | Mapped inside the 64 KB physical IRAM boundary. |
| **Resolution** | 160 x 144 @ 59.7 Hz | 224 x 144 @ 75.4 Hz | Exact 144p vertical match. Dual screen mode support:<br>- `STANDARD`: 160x144 centered with 32 px black borders.<br>- `WIDESCREEN`: 224x144 full-width for decompiled widescreen ports. |
| **Tile Format** | 2BPP Interleaved (2 bytes per row) | 2BPP Planar (8 bytes plane 0, 8 bytes plane 1) | Real-time byte transpose de-interleaver. |
| **Sprites** | 40 OAM sprites (8x8 or 8x16) | 128 hardware sprites (8x8) | 8x8 sprites mapped 1:1; 8x16 sprites split into two 8x8 sprites. |
| **Window** | 1 Window layer (`WX`, `WY`) | `Screen 2` hardware tilemap | Projected directly onto independent `Screen 2`. |
| **Color** | 15-bit BGR555 | 12-bit RGB444 (4096 colors) | Real-time bitshift downsampling: `r >> 1`, `g >> 1`, `b >> 1`. |
| **Input** | 8 buttons (D-Pad, A, B, Select, Start) | 11 buttons (X1-X4, Y1-Y4, A, B, Start) | Active-low matrix ($FF00). Y1 maps to Select. Y4 is assigned as a dedicated hot-toggle button to switch between 160x144 (32 px borders) and 224x144 widescreen on the fly. |
| **Mappers** | MBC1 (up to 2 MB), MBC5 (up to 8 MB) | Cartridge ROM Banks | Auto-detected from header byte `$0147` with bank mirroring. |

---

## Project Structure

```
WS/
├── .gitmodules                 # Submodule reference to arcanite24/gb-recompiled
├── submodules/
│   └── gb-recompiled/          # Upstream static recompiler source
├── include/
│   ├── gbc_types.h             # GB register definitions, flags, and constants
│   ├── gbc_memory.h            # Memory bus, MBC1/MBC5 banking, and RAM state
│   ├── gbc_video.h             # PPU scanner, tile translation, sprites, and palettes
│   ├── gbc_input.h             # Joypad polling and register $FF00 key matrix
│   ├── gbc_hal.h               # Core lifecycle, timers, and 60 Hz / 75 Hz sync
│   └── wsc_hardware.h          # WonderSwan Color ports and color conversion
├── src/
│   ├── gbc_memory.c            # Memory bus read/write and mapper controllers
│   ├── gbc_video.c             # Bitplane transpose, OAM translation, and window mapping
│   ├── gbc_input.c             # Input matrix translation
│   └── gbc_hal.c               # Master HAL lifecycle, TIMA/DIV timers, vsync loop
├── tests/
│   └── test_runner.c           # Comprehensive unit tests (46 test cases)
├── tools/
│   ├── gb_recomp_bridge.py     # ROM header parser, validator, and WSC binary builder
│   └── pipeline.py             # End-to-end automation pipeline script
├── roms/
│   ├── .gitignore              # Ignores copyrighted binaries (*.gb, *.gbc)
│   └── README.md               # ROM directory instructions
└── Makefile                    # Build rules for tests and pipeline
```

---

## Prerequisites

* **C Compiler:** GCC (MinGW on Windows, or standard GCC on Linux/macOS).
* **Python:** Python 3.8+ (for pipeline scripts).
* **Git:** With submodule support enabled.

---

## Usage

### 1. Running Unit Tests

Run the test suite to verify memory, tile conversion, sprite translation, and mappers:

```bash
make test
```

Expected output:
```text
========================================
   GBC -> WSC COMPATIBILITY LAYER TESTS 
========================================

Results:
  Passed: 61
  Failed: 0

ALL TESTS PASSED SUCCESSFULLY (Hito 1, 2 & 4 Core verified).
```

### 2. Converting a Game (Bring Your Own ROM)

1. Place your legally owned `.gb` or `.gbc` ROM into the `roms/` directory (e.g. `roms/tetris.gb` or `roms/tetris_dx.gbc`).
2. Run the automated pipeline:

```bash
python tools/pipeline.py roms/tetris.gb
```

Or via Makefile:

```bash
make game ROM=roms/tetris.gb
```

3. The generated WonderSwan Color executable will appear in `dist/<game_title>.wsc`, ready to be loaded into desktop test environments (Mesen 2, Mednafen) or flashed to physical hardware.

---

## License

This compatibility layer is licensed under the MIT License.
Game Boy and Game Boy Color are registered trademarks of Nintendo. This project is an independent research implementation and is not affiliated with Nintendo or Bandai.
