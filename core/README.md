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
* **Mappers**: Automatic detection of MBC1 (up to 2 MB) and MBC5 (up to 8 MB) from header byte `$0147`.
* **Bank Mirroring**: Handles power-of-two mirroring for small ROMs (e.g. 64 KB 4-bank titles like Super Mario Land).
* **Save State**: Battery-backed SRAM mapped to cartridge RAM space.

### 2. Video and Palette Pipeline (`gbc_video.c`, `gbc_video.h`)
* **2BPP Bitplane Transpose**: Game Boy stores tiles in interleaved scanline pairs (2 bytes per row). WonderSwan Color requires planar format (8 bytes for plane 0, followed by 8 bytes for plane 1). Real-time byte de-interleaving converts tiles on write or VBlank.
* **Color Downsampling**: Downsamples 15-bit BGR555 to native 12-bit RGB444 (`r >> 1`, `g >> 1`, `b >> 1`).
* **Sprite Mapping**: 
  * 8x8 sprites map 1:1 to WSC OAM.
  * 8x16 sprites are automatically split into two 8x8 WSC sprites (+8 Y offset).
* **Window Layer**: Projects GB Window (`WX`, `WY`) directly onto WSC hardware `Screen 2`.
* **Dual Screen Modes**:
  * `GBC_SCREEN_MODE_STANDARD`: 160x144 centered with 32 px black borders on each side. Culls sprites from bleeding into borders.
  * `GBC_SCREEN_MODE_WIDESCREEN`: 224x144 full-width viewport with 0 px offset, allowing sprites and camera to use all 28 horizontal tiles.

### 3. Input Subsystem (`gbc_input.c`, `gbc_input.h`)
* **Key Matrix**: Translates WonderSwan 16-bit key register (Port `$B5`) into Game Boy active-low register `$FF00`.
* **Default Mapping**:
  * Pad X (`X1`, `X2`, `X3`, `X4`) -> D-Pad (Up, Right, Down, Left).
  * `A` / `B` -> A / B.
  * `START` -> Start.
  * `Y1` -> Select.
  * `Y4` -> Dedicated hardware button to hot-toggle between Standard (32 px borders) and Widescreen (224 px) modes in real time.

### 4. Frame Pacing and Timers (`gbc_hal.c`, `gbc_hal.h`)
* **4:5 Cadence (Pacing)**: Compensates for the frequency difference between Game Boy (59.73 Hz) and WonderSwan Color (75.47 Hz). On 4 out of 5 hardware scans, the game logic and VBlank interrupt advance; on the 5th scan, game state is held. Resulting effective speed is 60.37 Hz (~1.0% from original, preserving physics and music tempo).
* **Timers**: Increments `DIV` counter every cycle and steps `TIMA` based on frequency configured in register `TAC`.

---

## Directory Layout

```
core/
├── include/
│   ├── gbc_types.h         # Game Boy registers, constants, and flags
│   ├── gbc_memory.h        # Memory bus and MBC declarations
│   ├── gbc_video.h         # Video pipeline, screen modes, and sprites
│   ├── gbc_input.h         # Input matrix and key processing
│   ├── gbc_hal.h           # Main HAL lifecycle, timers, and pacing
│   └── wsc_hardware.h      # WonderSwan Color ports and palette definitions
├── src/
│   ├── gbc_memory.c        # Bus implementation and MBC controllers
│   ├── gbc_video.c         # Planar transpose, sprite translation, palettes
│   ├── gbc_input.c         # Port $FF00 polling and Y4 button detection
│   └── gbc_hal.c           # Master VBlank loop and timer emulation
├── tests/
│   └── test_runner.c       # Comprehensive unit test suite (61 tests)
├── Makefile                # Library build and test targets
└── README.md               # This documentation
```

---

## Building and Testing

### 1. Run Unit Tests
```bash
make test
```
Compiles and executes the 61 unit tests covering memory banking, 2BPP conversion, DMA, sprite translation, window layers, timers, 4:5 pacing, dual screen modes, and the Y4 toggle button.

### 2. Build Static Library
```bash
make lib
```
Generates `libgbc_wsc.a` for linking with recompiled game code in `ports/`.
