# GBC to WonderSwan Color Compatibility Layer (HAL)

[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-Support%20Project-FF5E5B?style=flat&logo=kofi&logoColor=white)](https://ko-fi.com/rafadono)

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
gbc-to-wsc/
├── core/                       # Shared GBC -> WonderSwan Color HAL
│   ├── include/                # gbc_types.h, gbc_memory.h, gbc_video.h, gbc_input.h...
│   ├── src/                    # gbc_memory.c, gbc_video.c, gbc_input.c, gbc_hal.c
│   ├── tests/                  # Hardware abstraction unit tests (61 tests)
│   ├── Makefile                # Core library (libgbc_wsc.a) and test targets
│   └── README.md               # Dedicated HAL technical documentation
├── tools/                      # Build pipeline, packaging, and test runners
│   ├── bin/                    # Local tool binaries (gbrecomp)
│   ├── runner/                 # Desktop test runner drop folder (mednafen, Mesen)
│   ├── gb_recomp_bridge.py     # Header validation and WSC cartridge builder
│   ├── pipeline.py             # End-to-end automation pipeline script
│   └── test_wsc.py             # Desktop test runner and validation script
├── roms/                       # Untracked ROM directory (BYOR pattern)
│   ├── .gitignore              # Ignores copyrighted binaries (*.gb, *.gbc)
│   └── README.md               # ROM directory instructions
├── submodules/
│   └── gb-recompiled/          # Upstream static recompiler submodule
└── Makefile                    # Root build and orchestration rules
```

---

## Integration as a Git Submodule

To use `gbc-to-wsc` as the underlying translation HAL in a standalone game port repository:

```bash
git submodule add https://github.com/rafadono/gbc-to-wsc.git submodules/gbc-to-wsc
```

In your game's `Makefile`, link against the core library:

```makefile
CFLAGS += -Isubmodules/gbc-to-wsc/core/include
LDFLAGS += -Lsubmodules/gbc-to-wsc/core -lgbc_wsc
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

3. The generated WonderSwan Color executable will appear in `dist/<game_title>.wsc`.

### 3. Automated Validation and Desktop Execution

Place any WonderSwan desktop runner executable (e.g. `mednafen.exe`, `Mesen.exe`) inside the `tools/runner/` directory. The test system will auto-detect it:

```bash
# List discovered test runners
python tools/test_wsc.py --list

# Launch generated binary interactively
python tools/test_wsc.py dist/super_marioland.wsc

# Run automated validation test (monitors execution for 4s, verifies no crash)
python tools/test_wsc.py dist/super_marioland.wsc --validate
```

Or via Makefile:

```bash
make run ROM=dist/super_marioland.wsc
make validate ROM=dist/super_marioland.wsc
```

---

## Support

If you enjoy the project and want to support its ongoing development:

[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-Support%20Project-FF5E5B?style=for-the-badge&logo=kofi&logoColor=white)](https://ko-fi.com/rafadono)

---

## License

This compatibility layer is licensed under the MIT License.
Game Boy and Game Boy Color are registered trademarks of Nintendo. This project is an independent research implementation and is not affiliated with Nintendo or Bandai.
