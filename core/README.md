# GBC to WonderSwan Color Core Translation Layer (HAL)

A reusable, game-agnostic Hardware Abstraction Layer (HAL) and translation runtime that bridges statically recompiled Game Boy (GB) and Game Boy Color (GBC) guest logic to native Bandai WonderSwan Color (WSC) hardware.

This module is completely decoupled from any game title and compiles into a standalone static library (`libgbc_wsc.a`).

---

## Architecture Overview

```
+-------------------------------------------------------------+
|        Recompiled / Decompiled Guest Logic (C Source)       |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                 GBC to WSC CORE HAL (core/)                 |
|                                                             |
|  [gbc_memory] Memory Bus, MBC1/MBC5 Banking, IRAM Management|
|  [gbc_video]  2BPP Planar Transpose, RGB444 Palettes, OAM   |
|  [gbc_input]  $FF00 Key Matrix & Y4 Hot-Toggle Screen Mode  |
|  [gbc_hal]    Timers (DIV/TIMA), 4:5 Cadence (60 Hz Sync)   |
|  [wsc_hw]     Hardware Register Mapping & Color Conversion  |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|           WonderSwan Color Hardware / V30MZ Target          |
+-------------------------------------------------------------+
```

---

## Subsystem Specifications

### 1. Memory and Banking (`gbc_memory.c`, `gbc_memory.h`)
* **Unified IRAM**: Maps 32 KB WRAM and 16 KB VRAM within the WonderSwan Color's 64 KB physical IRAM boundary.
* **Mappers**:
  * **MBC1**: Up to 2 MB ROM and 32 KB RAM, supporting 16/4 and 4/32 modes.
  * **MBC2**: Up to 256 KB ROM with 512x4-bit internal RAM.
  * **MBC3**: Up to 2 MB ROM, 32 KB RAM, and Real-Time Clock (RTC) latching and registers.
  * **MBC5**: Up to 8 MB ROM and 128 KB RAM.
* **Bank Mirroring**: Handles power-of-two mirroring for small ROMs (e.g. 64 KB 4-bank titles like Super Mario Land).
* **Save State**: Battery-backed SRAM mapped to cartridge RAM space.

### 2. Video, Palette and HDMA Pipeline (`gbc_video.c`, `gbc_video.h`)
* **2BPP Bitplane Transpose**: Real-time byte de-interleaving converts GB interleaved tiles into WSC planar format.
* **CGB Dynamic Palettes**: Intercepts `BCPS`/`BCPD` and `OCPS`/`OCPD` with auto-increment, converting 15-bit BGR555 to native 12-bit RGB444 (`r >> 1`, `g >> 1`, `b >> 1`).
* **DMG 27-Preset Palette Engine**: Provides 27 built-in color presets for monochrome games, including classic hardware (DMG-01, Pocket, Light teal, Virtual Boy crimson), the 12 official GBC boot ROM combinations, Super Game Boy (1A, 2A, 4G), and community favorites (Warm Sepia, Cool Blue, CRT Amber, CRT Green/Matrix, Cyberpunk Neon, Frostbite, Crimson Night, WSC Vibrant). Cycled via button `Y2` in real time or selected via API.
* **HDMA Transfers**: Implements General Purpose DMA (instant copy to VRAM) and HBlank DMA (16 bytes per scanline) via `HDMA1-HDMA5`.
* **Sprite Mapping**: 
  * 8x8 sprites map 1:1 to WSC OAM.
  * 8x16 sprites are automatically split into two 8x8 WSC sprites (+8 Y offset).
* **Window Layer (Screen 2)**: Maps Game Boy Window (`WX`, `WY`) directly onto WSC hardware `Screen 2`, utilizing hardware clipping registers (`0x30 - 0x33`).
* **Dual Screen Modes & Tate Orientation**:
  * `GBC_SCREEN_MODE_STANDARD`: 160x144 centered with 32 px black borders on each side. Culls sprites from bleeding into borders.
  * `GBC_SCREEN_MODE_WIDESCREEN`: 224x144 full-width viewport with 0 px offset, allowing sprites and camera to use all 28 horizontal tiles.
  * **Tate Vertical Mode (90 degrees)**: Rotates presentation to 144x224 vertical orientation and remaps controls for physical vertical gameplay.

### 3. Audio Subsystem (`gbc_audio.c`, `gbc_audio.h`)
* **APU to WSC Digital Sound Generator**: Translates Game Boy audio channels to WonderSwan Color hardware audio ports (`$80 - $9E`).
* **Channels 1 & 2 (Pulse)**: Pitch frequency scaling ($P_{WSC} = 2048 - (2048 - P_{GB}) \times 375 / 512$), volume packing, envelope and sweep mirroring.
* **Acoustic Harmonic Smoothing**: Band-limited edge-smoothing table (`s_duty_enhanced`) for WonderSwan Voice RAM, eliminating harsh high-frequency aliasing distortion on physical hardware speakers while preserving tempo and pitch. Fully toggleable to strict raw 1:1 square waves.
* **Channel 3 (Wave)**: Pitch frequency scaling ($P_{WSC} = 2048 - (2048 - P_{GB}) \times 375 / 256$), 4-level volume shifting, and 32 4-bit sample Wave RAM transfer (`$FF30 - $FF3F`).
* **Channel 4 (Noise)**: Polynomial frequency and 7/15-bit LFSR translation mapped to WSC noise control port `$8C`.
* **Master Controls**: Panning (`NR51` -> Port `$90`) and Master Volume (`NR50` -> Port `$91`).

### 4. Input Subsystem (`gbc_input.c`, `gbc_input.h`)
* **Key Matrix**: Translates WonderSwan 16-bit key register (Port `$B5`) into Game Boy active-low register `$FF00`.
* **Default Landscape Mapping**:
  * Pad X (`X1`, `X2`, `X3`, `X4`) -> D-Pad (Up, Right, Down, Left).
  * `A` / `B` -> A / B.
  * `START` -> Start.
  * `Y1` -> Select.
  * `Y2` -> DMG Palette Cycler (when enabled).
  * `Y4` -> Widescreen hot-toggle (when enabled).
* **Tate Vertical Mapping**:
  * Pad Y (`Y2`, `Y3`, `Y4`, `Y1`) -> D-Pad (Up, Right, Down, Left).
  * Pad X (`X4`, `X3`) -> A / B.
  * `START` -> Start.
  * `A` -> Select.

### 5. Modular Feature Flags (`gbc_features_t`)
All runtime enhancements are modular and can be individually disabled:
* `gbc_features_set_widescreen_toggle(bool)`: Enables or disables `Y4` widescreen switching.
* `gbc_features_set_dmg_palette_cycle(bool)`: Enables or disables `Y2` DMG palette cycling.
* `gbc_features_set_tate_mode(bool)`: Toggles 90-degree Tate vertical mode.
* `gbc_features_set_enhanced_audio(bool)`: Toggles acoustic harmonic waveform smoothing.

### 6. Frame Pacing and Timers (`gbc_hal.c`, `gbc_hal.h`)
* **4:5 Cadence (Pacing)**: Compensates for the frequency difference between Game Boy (59.73 Hz) and WonderSwan Color (75.47 Hz). On 4 out of 5 hardware scans, the game logic and VBlank interrupt advance; on the 5th scan, game state is held. Resulting effective speed is 60.37 Hz (~1.0% from original, preserving physics and music tempo).
* **Timers**: Increments `DIV` counter every cycle and steps `TIMA` based on frequency configured in register `TAC`.

---

## Directory Layout

```
core/
├── include/
│   ├── gbc_types.h         # Game Boy registers, constants, and flags
│   ├── gbc_memory.h        # Memory bus, MBC1/2/3/5 declarations
│   ├── gbc_video.h         # Video pipeline, screen modes, CGB palettes, HDMA
│   ├── gbc_audio.h         # APU to WSC Sound Generator translation
│   ├── gbc_input.h         # Input matrix and key processing
│   ├── gbc_hal.h           # Main HAL lifecycle, timers, and pacing
│   └── wsc_hardware.h      # WonderSwan Color ports and palette definitions
├── src/
│   ├── gbc_memory.c        # Bus implementation and MBC controllers
│   ├── gbc_video.c         # Planar transpose, sprites, CGB palettes, HDMA, Screen 2
│   ├── gbc_audio.c         # Audio pitch calculation, volume packing, Wave RAM
│   ├── gbc_input.c         # Port $FF00 polling and Y4 button detection
│   └── gbc_hal.c           # Master VBlank loop, timer and audio stepping
├── tests/
│   └── test_runner.c       # Comprehensive unit test suite
├── Makefile                # Library build and test targets
└── README.md               # Dedicated HAL documentation
```

---

## Building and Testing

### 1. Run Unit Tests
```bash
make test
```
Compiles and executes the unit tests covering memory banking (MBC1, MBC2, MBC3 RTC, MBC5), 2BPP conversion, DMA, CGB dynamic palettes, HDMA transfers, sprite translation, Screen 2 window clipping, timers, 4:5 pacing, dual screen modes, and audio translation.

### 2. Build Static Library
```bash
make lib
```
Generates `libgbc_wsc.a` for linking with recompiled game code in standalone ports.
