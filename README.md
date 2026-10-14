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
|  - Memory & Mappers (MBC1, MBC2, MBC3+RTC, MBC5)      |
|  - PPU & CGB HDMA (General Purpose & HBlank DMA)      |
|  - Palettes (DMG & CGB BGR555 -> WSC RGB444)          |
|  - Sprites (8x8 & 8x16 OAM -> 128 WSC Sprites)        |
|  - Window Layer (GB Window -> WSC Screen 2 Clipping)  |
|  - Audio Engine (APU 4-Ch -> WSC Sound Ports $80-$9E) |
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
| **CPU** | Sharp SM83 (Z80-hybrid 8-bit) @ 4.19 / 8.38 MHz (~1.05 - 2.1 MIPS) | NEC V30MZ (16-bit CISC pipelined x86) @ 3.072 MHz (~2.2 - 2.8 MIPS) | 16-bit instruction coalescing (paired memory ops into single words), single-cycle pointer math (`[BX + SI]`), and 16-bit block copies (`REP MOVSW`). |
| **RAM** | 32 KB WRAM + 16 KB VRAM (~48 KB) | 64 KB Unified IRAM | Mapped inside the 64 KB physical IRAM boundary. |
| **Resolution** | 160 x 144 @ 59.7 Hz | 224 x 144 @ 75.4 Hz | Exact 144p vertical match. Dual screen mode support:<br>- `STANDARD`: 160x144 centered with 32 px black borders.<br>- `WIDESCREEN`: 224x144 full-width for decompiled widescreen ports. |
| **Tile Format** | 2BPP Interleaved (2 bytes per row) | 2BPP Planar (8 bytes plane 0, 8 bytes plane 1) | Real-time byte transpose de-interleaver. |
| **Sprites** | 40 OAM sprites (8x8 or 8x16) | 128 hardware sprites (8x8) | 8x8 sprites mapped 1:1; 8x16 sprites split into two 8x8 sprites. |
| **Window** | 1 Window layer (`WX`, `WY`) | `Screen 2` hardware tilemap | Projected directly onto independent `Screen 2` with hardware clipping (`0x30 - 0x33`). |
| **Color** | 15-bit BGR555 | 12-bit RGB444 (4096 colors) | Real-time bitshift downsampling: `r >> 1`, `g >> 1`, `b >> 1` via `BCPS`/`BCPD` and `OCPS`/`OCPD`. |
| **HDMA** | General Purpose & HBlank DMA (`HDMA1-5`) | Block Memory Copy | 16 bytes per HBlank scanline or full burst copy to VRAM bank 0/1. |
| **Audio** | 4-channel APU (2 Pulse, 1 Wave, 1 Noise) | 4-channel Sound Generator | Frequency scaling, volume packing, Wave RAM sync, and noise LFSR mapping. |
| **Input** | 8 buttons (D-Pad, A, B, Select, Start) | 11 buttons (X1-X4, Y1-Y4, A, B, Start) | Active-low matrix ($FF00). Y1 maps to Select. Y4 is assigned as a dedicated hot-toggle button to switch between 160x144 (32 px borders) and 224x144 widescreen on the fly. |
| **Mappers** | MBC1 (2 MB), MBC2 (256 KB), MBC3+RTC (2 MB), MBC5 (8 MB) | Cartridge ROM Banks | Auto-detected from header byte `$0147` with bank mirroring and RTC latching. |

---

## 16-Bit CISC Processor Acceleration and Instruction Coalescing

The Bandai WonderSwan Color's **NEC V30MZ** processor delivers significant performance benefits over the original 8-bit Sharp SM83 CPU:

1. **Instruction Coalescing (Pairing 8-Bit Operations into 16-Bit Words)**:
   * In the SM83, manipulating 16-bit variables (such as 16-bit positions or pointers) required sequential 8-bit loads and stores (`LD A, [hl+] / LD [de], A / LD A, [hl] / LD [de], A`, taking 16 T-states).
   * In recompiled code, contiguous 8-bit operations are coalesced into single 16-bit word instructions (`MOV AX, [SI] / MOV [DI], AX`), reducing executed instructions and cycle consumption by **50% to 70%**.
2. **Single-Cycle 16-Bit Pointer Math (`[BX + SI]`)**:
   * SM83 pointer additions (`ADD HL, BC`) required 8 T-states (2 machine cycles = ~1.90 µs).
   * The V30MZ executes register-to-register pointer arithmetic in **1 clock cycle** (0.32 µs, nearly **6x faster**).
3. **16-Bit Block Transfers (`REP MOVSW` / `REP STOSW`)**:
   * OAM updates, VRAM tile pattern transfers, and Wave RAM syncing utilize 16-bit word string transfers, moving **2 bytes per bus cycle** and reducing memory bus overhead by **50%**.
4. **Hardware-Accelerated Pitch Math**:
   * Frequency translation ($P_{WSC} = 2048 - (2048 - P_{GB}) \times 375 / 512$) executes in **8 to 10 cycles** on the V30MZ using native 16-bit multiplication (`MUL`) and barrel shifts, compared to ~90 to 110 cycles on the 8-bit SM83.
5. **Software PCM Audio Streaming (Digitized Speech and Samples)**:
   * Titles featuring digital speech or PCM percussion (*Pokémon Yellow*, *Street Fighter Alpha*, *Resident Evil Gaiden*) that consumed 25% - 35% of the SM83 CPU execute at **~7.2% CPU** via 16-bit SWAR sample unpacking.

---

## Project Structure

```
gbc-to-wsc/
├── core/                       # Shared GBC -> WonderSwan Color HAL
│   ├── include/                # gbc_types.h, gbc_memory.h, gbc_video.h, gbc_audio.h...
│   ├── src/                    # gbc_memory.c, gbc_video.c, gbc_audio.c, gbc_input.c, gbc_hal.c
│   ├── tests/                  # Hardware abstraction unit tests
│   ├── Makefile                # Core library (libgbc_wsc.a) and test targets
│   └── README.md               # Dedicated HAL technical documentation
├── tools/                      # Build pipeline, packaging, and test runners
│   ├── bin/                    # Local tool binaries (gbrecomp)
│   ├── runner/                 # Desktop test runner drop folder (mednafen, Mesen)
│   ├── gb_recomp_bridge.py     # Header validation and WSC cartridge builder
│   ├── pipeline.py             # End-to-end automation pipeline script
│   ├── new_port.py             # Standalone game port repository scaffolding tool
│   ├── test_core.py            # Core subsystems validation test harness
│   └── test_wsc.py             # Desktop test runner and validation script
├── roms/                       # Untracked ROM directory (BYOR pattern)
│   ├── .gitignore              # Ignores copyrighted binaries (*.gb, *.gbc)
│   └── README.md               # ROM directory instructions
├── submodules/
│   └── gb-recompiled/          # Upstream static recompiler submodule
└── Makefile                    # Root build and orchestration rules
```

---

## Scaffolding a New Game Port

To automatically scaffold a new standalone game repository configured with hooks, submodules, and customizable feature flags:

```bash
python tools/new_port.py "roms/Metroid II (World).gb" ../metroid2-wsc --dmg-palettes --widescreen
```

Available CLI flags:
* `--widescreen`: Enables the real-time widescreen toggle on button `Y4`.
* `--dmg-palettes`: Enables the 27-preset DMG palette cycler on button `Y2`.
* `--tate`: Configures Tate 90-degree vertical orientation and remapped controls.
* `--no-enhanced-audio`: Disables harmonic waveform smoothing, enforcing strict raw pulse duty cycles.
* `--smooth-camera`: Enables 75 fps smooth background camera interpolation on 4:5 cadence held frames.

---

## Configurable Feature Flags & Palettes

All enhancements in `gbc-to-wsc` are modular and can be individually enabled or disabled via C API or `config.json`:

```c
/* In your game's hooks_init callback: */
gbc_features_set_widescreen_toggle(false); // Disables Y4 widescreen toggle (keeps strict 160x144)
gbc_features_set_dmg_palette_cycle(true);  // Enables Y2 DMG palette cycler across 27 presets
gbc_features_set_tate_mode(false);         // Disables Tate mode (landscape orientation)
gbc_features_set_enhanced_audio(true);     // Enables WonderSwan acoustic harmonic smoothing
gbc_features_set_smooth_camera(true);      // Enables 75 fps smooth background camera interpolation
```

### DMG Palette Catalog (27 Presets)

The real-time palette engine (cycled with `Y2` on monochrome titles or selected with `gbc_video_set_dmg_palette_preset()`) provides 27 built-in presets:

1. **Classic Hardware**:
   * `DMG_PALETTE_ORIGINAL_GREEN`: Original 1989 Game Boy DMG-01 pea-soup green LCD matrix.
   * `DMG_PALETTE_POCKET_GRAYSCALE`: Game Boy Pocket crisp neutral grayscale.
   * `DMG_PALETTE_LIGHT_TEAL`: Game Boy Light electroluminescent teal/aqua glow.
   * `DMG_PALETTE_VIRTUAL_BOY`: Virtual Boy deep crimson LED on pure black.
2. **Official GBC Boot ROM Shortcuts**:
   * `DMG_PALETTE_GBC_BROWN` (Up): Warm ochre brown.
   * `DMG_PALETTE_GBC_RED` (Up+A): Vivid crimson and salmon red.
   * `DMG_PALETTE_GBC_DARK_BROWN` (Up+B): Dark amber brown.
   * `DMG_PALETTE_GBC_PASTEL` (Down): Pastel sky blue and soft orange mix.
   * `DMG_PALETTE_GBC_ORANGE` (Down+A): Warm amber orange and gold.
   * `DMG_PALETTE_GBC_YELLOW` (Down+B): Bright golden yellow and olive.
   * `DMG_PALETTE_GBC_BLUE` (Left): Vibrant royal blue and sky blue.
   * `DMG_PALETTE_GBC_DARK_BLUE` (Left+A): Midnight deep navy and steel blue.
   * `DMG_PALETTE_GBC_GRAYSCALE` (Left+B): GBC high-contrast grayscale.
   * `DMG_PALETTE_GBC_GREEN` (Right): Emerald green and light lime.
   * `DMG_PALETTE_GBC_DARK_GREEN` (Right+A): Forest green and deep moss.
   * `DMG_PALETTE_GBC_INVERTED` (Right+B): Inverted negative monochrome.
3. **Super Game Boy (SGB)**:
   * `DMG_PALETTE_SGB_1A`: Classic SGB olive-gold.
   * `DMG_PALETTE_SGB_2A`: SGB fiery sunset orange-red.
   * `DMG_PALETTE_SGB_4G`: SGB soft pastel candy.
4. **Community Favorites (Lospec, RetroArch, Shaders)**:
   * `DMG_PALETTE_WARM_SEPIA`: Vintage photographic sepia tone.
   * `DMG_PALETTE_COOL_BLUE`: High-readability iced blue gradient.
   * `DMG_PALETTE_CRT_AMBER`: Classic amber phosphor terminal CRT monitor.
   * `DMG_PALETTE_CRT_GREEN`: Matrix phosphor green terminal monitor.
   * `DMG_PALETTE_CYBERPUNK_NEON`: Synthwave neon cyan, hot magenta, and midnight purple.
   * `DMG_PALETTE_FROSTBITE`: Glacial frostbite arctic blue.
   * `DMG_PALETTE_CRIMSON_NIGHT`: Gothic blood red and black.
   * `DMG_PALETTE_WSC_VIBRANT`: Tailored for WonderSwan Color reflective FSTN/TFT display.

---

## Integration as a Git Submodule

To manually link `gbc-to-wsc` in an existing game repository:

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
