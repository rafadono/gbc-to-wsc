# Super Mario Land - WonderSwan Color Port Module

Game-specific port module and enhancement hooks for **Super Mario Land** on the Bandai WonderSwan Color, linked against the shared `core/` HAL.

---

## Target ROM Specifications

| Property | Value |
| :--- | :--- |
| **Title** | Super Mario Land |
| **Revision** | World (Rev A / v1.1) |
| **SHA-1 Hash** | `418203621B887CAA090215D97E3F509B79AFFD3E` |
| **ROM Size** | 65,536 bytes (64 KB, 4 banks of 16 KB) |
| **Mapper** | MBC1 (with bank power-of-two mirroring) |
| **Original Resolution** | 160 x 144 @ 59.73 Hz |

---

## Module Overview and Specific Adaptations

While the generic Stage 1 pipeline runs the game in standard 160x144 mode with 32 px black borders, this module provides **Stage 2 native enhancements**:

### 1. Widescreen Camera Streaming Hook (`sml_hooks.c`)
* In the original Game Boy code, the camera tile-streaming routine is hardcoded to 20 tiles wide (160 px).
* When widescreen mode is active, `sml_hook_get_camera_width_tiles()` requests streaming for **28 tiles (224 px)**, eliminating border tile popping.

### 2. Entity Activation Window (Enemy Spawning / Culling)
* In vanilla code, enemies and obstacles spawn and activate when the camera gets within 160 pixels.
* The hook `sml_hook_is_enemy_in_active_window()` widens the activation window to $X \le 224\text{ px}$, preventing enemies from appearing abruptly in the newly exposed side screen regions.

### 3. Top HUD Layout Centering
* Super Mario Land places the score, coin counter, stage indicator, and timer (`MARIO 000000 00 1-1 TIME 398`) in a single row across 20 tiles.
* In 224 px widescreen mode, `sml_hook_update_hud_layout()` centers the HUD row with a 4-tile margin on each side for balanced framing.

### 4. Real-Time Screen Mode Hot-Toggle
* The game hooks into the HAL's physical **`Y4` button**. Pressing `Y4` during gameplay toggles instantly between:
  * **Standard Mode**: 160x144 pixel-perfect display with 32 px black sidebars.
  * **Widescreen Mode**: 224x144 full-width display with adapted camera and entity bounds.

---

## Building and Verification

### 1. Place the ROM
Place `Super Mario Land (World) (Rev A).gb` in the `roms/` folder.

### 2. Build the Executable
Run the unified pipeline:
```bash
python tools/pipeline.py "roms/Super Mario Land (World) (Rev A).gb"
```
The output binary will be generated at:
`dist/super_marioland.wsc` (128 KB, Checksum `0x601D`).

### 3. Test and Validate
```bash
# Automated validation (4s non-crash test)
python tools/test_wsc.py dist/super_marioland.wsc --validate

# Interactive playtest
python tools/test_wsc.py dist/super_marioland.wsc
```

### 4. Applying and Testing the Widescreen Patch

Apply the widescreen patch to the decompiled C source files:
```bash
python ports/super_marioland/apply_widescreen_patch.py
```

Run the dedicated Super Mario Land widescreen test suite (13 tests):
```bash
make -C ports/super_marioland test
```

To revert back to vanilla camera values:
```bash
python ports/super_marioland/apply_widescreen_patch.py --revert
```
