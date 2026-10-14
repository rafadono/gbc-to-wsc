#!/usr/bin/env python3
"""
WonderSwan Color Port Generator Tool
Scaffolds a complete standalone game port repository linked against gbc-to-wsc.
"""

import sys
import os
import argparse
import json
import subprocess
import shutil

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(BASE_DIR, "tools"))
import gb_recomp_bridge

def create_port(rom_path, output_dir, title=None, widescreen=False, dmg_palettes=None, tate=False, enhanced_audio=True, smooth_camera=False, init_git=True):
    if not os.path.isfile(rom_path):
        print(f"Error: ROM file not found: {rom_path}")
        return False

    with open(rom_path, "rb") as f:
        rom_data = f.read()

    header = gb_recomp_bridge.parse_gb_header(rom_data)
    game_title = title or header["title"]
    game_slug = "".join(c if c.isalnum() else "_" for c in game_title.lower()).strip("_")
    is_cgb = header["is_cgb"]
    cart_type = header["cart_type"]

    if dmg_palettes is None:
        dmg_palettes = not is_cgb

    abs_output = os.path.abspath(output_dir)
    os.makedirs(abs_output, exist_ok=True)
    os.makedirs(os.path.join(abs_output, "hooks"), exist_ok=True)
    os.makedirs(os.path.join(abs_output, "roms"), exist_ok=True)
    os.makedirs(os.path.join(abs_output, "dist"), exist_ok=True)
    os.makedirs(os.path.join(abs_output, ".github"), exist_ok=True)

    print(f"--- Creating New WonderSwan Color Port: {game_title} ---")
    print(f"Target directory: {abs_output}")
    print(f"ROM Type: {cart_type} | CGB: {is_cgb}")

    # 1. config.json
    config_data = {
        "title": game_title,
        "slug": game_slug,
        "cart_type": cart_type,
        "is_cgb": is_cgb,
        "features": {
            "widescreen_toggle": widescreen,
            "dmg_palette_cycle": dmg_palettes,
            "tate_mode": tate,
            "enhanced_audio": enhanced_audio,
            "smooth_camera": smooth_camera
        }
    }
    with open(os.path.join(abs_output, "config.json"), "w", encoding="utf-8") as f:
        json.dump(config_data, f, indent=2)

    # 2. hooks/<slug>_hooks.h
    hooks_h_content = f"""#ifndef {game_slug.upper()}_HOOKS_H
#define {game_slug.upper()}_HOOKS_H

#include <stdint.h>
#include <stdbool.h>
#include "gbc_hal.h"

void {game_slug}_hooks_init(void);
void {game_slug}_hooks_on_frame(void);

#endif
"""
    with open(os.path.join(abs_output, "hooks", f"{game_slug}_hooks.h"), "w", encoding="utf-8") as f:
        f.write(hooks_h_content)

    # 3. hooks/<slug>_hooks.c
    hooks_c_content = f"""#include "{game_slug}_hooks.h"
#include <stdio.h>

void {game_slug}_hooks_init(void) {{
    gbc_features_set_widescreen_toggle({"true" if widescreen else "false"});
    gbc_features_set_dmg_palette_cycle({"true" if dmg_palettes else "false"});
    gbc_features_set_tate_mode({"true" if tate else "false"});
    gbc_features_set_enhanced_audio({"true" if enhanced_audio else "false"});
    gbc_features_set_smooth_camera({"true" if smooth_camera else "false"});
}}

void {game_slug}_hooks_on_frame(void) {{
    /* Game-specific frame callback hook */
}}
"""
    with open(os.path.join(abs_output, "hooks", f"{game_slug}_hooks.c"), "w", encoding="utf-8") as f:
        f.write(hooks_c_content)

    # 4. .gitignore
    gitignore_content = """# Binaries & build artifacts
*.exe
*.o
*.a
*.lib
*.obj
*.pdb
*.dll
*.so
*.dylib
*.wsc
build/
bin/
dist/

# ROM files & save data (Never commit copyrighted game files)
*.gb
*.gbc
roms/*.gb
roms/*.gbc
roms/*.bin
roms/*.sav
!roms/.gitkeep
!roms/README.md

# Python cache
__pycache__/
*.pyc
*.pyo

# Editor & OS files
.vscode/
.idea/
Thumbs.db
.DS_Store
"""
    with open(os.path.join(abs_output, ".gitignore"), "w", encoding="utf-8") as f:
        f.write(gitignore_content)

    # 5. .github/FUNDING.yml
    funding_content = "ko_fi: rafadono\n"
    with open(os.path.join(abs_output, ".github", "FUNDING.yml"), "w", encoding="utf-8") as f:
        f.write(funding_content)

    # 6. roms/README.md and .gitkeep
    with open(os.path.join(abs_output, "roms", ".gitkeep"), "w") as f:
        f.write("")
    with open(os.path.join(abs_output, "roms", "README.md"), "w", encoding="utf-8") as f:
        f.write(f"# Target ROM Directory\n\nPlace your legally dumped `{os.path.basename(rom_path)}` here.\n")

    # 7. Makefile
    makefile_content = f"""CC ?= gcc
AR ?= ar
CFLAGS = -Wall -Wextra -O2 -Isubmodules/gbc-to-wsc/core/include -Ihooks

HOOKS_SRC = hooks/{game_slug}_hooks.c
HOOKS_OBJ = $(HOOKS_SRC:.c=.o)
CORE_LIB = submodules/gbc-to-wsc/core/libgbc_wsc.a

all: build

build: $(HOOKS_OBJ)
\t@echo "Build complete for {game_title}."

submodule-init:
\tgit submodule update --init --recursive

clean:
\trm -f hooks/*.o dist/*.wsc

.PHONY: all build submodule-init clean
"""
    with open(os.path.join(abs_output, "Makefile"), "w", encoding="utf-8") as f:
        f.write(makefile_content)

    # 8. README.md
    readme_content = f"""# {game_title} - WonderSwan Color Port

[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-Support%20Project-FF5E5B?style=flat&logo=kofi&logoColor=white)](https://ko-fi.com/rafadono)

Standalone port of **{game_title}** to the Bandai WonderSwan Color, powered by the [`gbc-to-wsc`](https://github.com/rafadono/gbc-to-wsc) hardware abstraction layer.

---

## Specifications

* **Cartridge Type**: `{cart_type}`
* **Color Mode**: `{"Game Boy Color (CGB)" if is_cgb else "Monochrome (DMG)"}`
* **Widescreen Mode**: `{"Enabled (Y4 hot-toggle)" if widescreen else "Disabled (Standard 160x144)"}`
* **DMG Palettes**: `{"Enabled (Y2 hot-cycle)" if dmg_palettes else "Disabled"}`
* **Tate Mode (90° Vertical)**: `{"Enabled" if tate else "Disabled"}`
* **Enhanced Audio**: `{"Enabled" if enhanced_audio else "Disabled"}`

---

## Building and Verification

1. Place your ROM in `roms/`:
   ```bash
   cp "{os.path.basename(rom_path)}" roms/
   ```

2. Initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

3. Build and package:
   ```bash
   python submodules/gbc-to-wsc/tools/pipeline.py "roms/{os.path.basename(rom_path)}"
   ```

---

## Support

[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-Support%20Project-FF5E5B?style=for-the-badge&logo=kofi&logoColor=white)](https://ko-fi.com/rafadono)
"""
    with open(os.path.join(abs_output, "README.md"), "w", encoding="utf-8") as f:
        f.write(readme_content)

    # 9. Initialize Git and Submodule if requested
    if init_git:
        try:
            subprocess.run(["git", "init", "-b", "main"], cwd=abs_output, capture_output=True, check=True)
            subprocess.run(["git", "submodule", "add", "https://github.com/rafadono/gbc-to-wsc.git", "submodules/gbc-to-wsc"], cwd=abs_output, capture_output=True, check=True)
            subprocess.run(["git", "add", "."], cwd=abs_output, capture_output=True, check=True)
            subprocess.run(["git", "commit", "-m", f"feat: initial commit for {game_title} WonderSwan Color port"], cwd=abs_output, capture_output=True, check=True)
            print("Successfully initialized Git repository and linked gbc-to-wsc submodule.")
        except Exception as ex:
            print(f"Notice: Git initialization notice: {ex}")

    print(f"Port generation complete at: {abs_output}")
    return True

def main():
    parser = argparse.ArgumentParser(description="WonderSwan Color Port Generator")
    parser.add_argument("rom", help="Path to Game Boy or Game Boy Color ROM")
    parser.add_argument("output", help="Target output directory")
    parser.add_argument("--title", help="Custom game title")
    parser.add_argument("--widescreen", action="store_true", help="Enable widescreen hot-toggle")
    parser.add_argument("--dmg-palettes", action="store_true", default=None, help="Enable Y2 DMG palette cycler")
    parser.add_argument("--tate", action="store_true", help="Enable Tate 90-degree vertical orientation")
    parser.add_argument("--no-enhanced-audio", action="store_false", dest="enhanced_audio", default=True, help="Disable enhanced audio harmonic smoothing")
    parser.add_argument("--smooth-camera", action="store_true", help="Enable 75 fps smooth background camera interpolation")
    parser.add_argument("--no-git", action="store_false", dest="init_git", default=True, help="Skip git initialization and submodule addition")

    args = parser.parse_args()
    success = create_port(
        rom_path=args.rom,
        output_dir=args.output,
        title=args.title,
        widescreen=args.widescreen,
        dmg_palettes=args.dmg_palettes,
        tate=args.tate,
        enhanced_audio=args.enhanced_audio,
        smooth_camera=args.smooth_camera,
        init_git=args.init_git
    )
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
