# Tetris DX - WonderSwan Color Port Module

Game-specific port module and enhancement hooks for **Tetris DX** (Game Boy Color) on the Bandai WonderSwan Color, linked against the shared `core/` HAL.

---

## Target ROM Specifications

| Property | Value |
| :--- | :--- |
| **Title** | Tetris DX |
| **Region** | World |
| **SHA-1 Hash** | `7183BCB54DD35F3A07D8FE63339B768F13B8168D` |
| **ROM Size** | 524,288 bytes (512 KB, 32 banks of 16 KB) |
| **Mapper** | MBC5 (with 32 KB battery-backed SRAM) |
| **Color Support** | Game Boy Color (CGB) Enhanced |

---

## Module Overview and Specific Adaptations

### 1. Matrix Preservation and Framing (Standard Mode)
* Tetris gameplay rules rely on a strict 10x20 tile playfield. Expanding the playfield horizontally to 224 pixels would alter the block drop physics and competitive line-clearing mechanics.
* This module locks default presentation to `GBC_SCREEN_MODE_STANDARD` (160x144 centered with 32 px black borders), ensuring 100% competitive parity with the original Game Boy Color release.

### 2. Color Space Translation (BGR555 -> RGB444)
* Game Boy Color encodes palettes in 15-bit BGR555 format (32,768 colors).
* WonderSwan Color hardware palette registers accept 12-bit RGB444 (4,096 colors).
* The palette pipeline translates CGB palette RAM entries (`$FF68-$FF6B`) in real time using bitshift downsampling:
  $$\text{R}_{4} = \text{R}_{5} \gg 1, \quad \text{G}_{4} = \text{G}_{5} \gg 1, \quad \text{B}_{4} = \text{B}_{5} \gg 1$$
  This yields vibrant color reproduction on the WonderSwan Color STN LCD.

### 3. Battery-Backed SRAM Persistence
* Tetris DX saves high scores, line records, and power-on rankings into 32 KB cartridge SRAM (MBC5 bank-switched RAM).
* The hook `tetris_dx_hook_sync_save_ram()` maps guest SRAM writes to the WonderSwan Color Cartridge backup RAM area.

---

## Building and Verification

### 1. Place the ROM
Place `Tetris DX (World).gbc` in the `roms/` folder.

### 2. Build the Executable
Run the unified pipeline:
```bash
python tools/pipeline.py "roms/Tetris DX (World).gbc"
```
The output binary will be generated at:
`dist/tetris_dx.wsc` (128 KB, Checksum `0x813C`).

### 3. Test and Validate
```bash
# Automated validation (4s non-crash test)
python tools/test_wsc.py dist/tetris_dx.wsc --validate

# Interactive playtest
python tools/test_wsc.py dist/tetris_dx.wsc
```
