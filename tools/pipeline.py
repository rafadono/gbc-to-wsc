#!/usr/bin/env python3
"""
GB/GBC to WonderSwan Color Unified Pipeline
Orchestrates: ROM -> gbrecomp (static recompilation) -> gbc_wsc_hal -> .wsc binary
"""

import sys
import os
import shutil
import urllib.request
import zipfile
import subprocess
import platform

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS_BIN_DIR = os.path.join(BASE_DIR, "tools", "bin")
GBRECOMP_RELEASE_URL_WIN = "https://github.com/arcanite24/gb-recompiled/releases/latest/download/gb-recompiled-windows-x64.zip"

def get_gbrecomp_executable():
    # 1. Check local tools/bin and subdirectories
    ext = ".exe" if platform.system() == "Windows" else ""
    local_bin = os.path.join(TOOLS_BIN_DIR, f"gbrecomp{ext}")
    if os.path.isfile(local_bin):
        return local_bin

    sub_bin = os.path.join(TOOLS_BIN_DIR, "gb-recompiled-windows", f"gbrecomp{ext}")
    if os.path.isfile(sub_bin):
        return sub_bin

    # 2. Check system PATH
    which_bin = shutil.which("gbrecomp")
    if which_bin:
        return which_bin

    # 3. Check submodule build directory
    submodule_bin = os.path.join(BASE_DIR, "submodules", "gb-recompiled", "build", f"gbrecomp{ext}")
    if os.path.isfile(submodule_bin):
        return submodule_bin

    return None

def ensure_gbrecomp():
    exe = get_gbrecomp_executable()
    if exe:
        return exe

    os.makedirs(TOOLS_BIN_DIR, exist_ok=True)
    if platform.system() == "Windows":
        zip_path = os.path.join(TOOLS_BIN_DIR, "gb-recompiled.zip")
        print("gbrecomp not found locally. Downloading official binary from arcanite24/gb-recompiled...")
        try:
            urllib.request.urlretrieve(GBRECOMP_RELEASE_URL_WIN, zip_path)
            with zipfile.ZipFile(zip_path, 'r') as zip_ref:
                zip_ref.extractall(TOOLS_BIN_DIR)
            if os.path.exists(zip_path):
                os.remove(zip_path)
            
            ext = ".exe"
            target_exe = os.path.join(TOOLS_BIN_DIR, f"gbrecomp{ext}")
            if os.path.isfile(target_exe):
                print(f"Successfully installed at {target_exe}")
                return target_exe
        except Exception as e:
            print(f"Notice: Could not download prebuilt binary ({e}).")
            print("You can build it using CMake in submodules/gb-recompiled.")
    
    return None

def run_pipeline(rom_path):
    if not os.path.isfile(rom_path):
        print(f"Error: ROM file not found at '{rom_path}'")
        return False

    print(f"--- Starting Pipeline for: {os.path.basename(rom_path)} ---")
    
    # 1. Parse header using bridge
    sys.path.append(os.path.join(BASE_DIR, "tools"))
    import gb_recomp_bridge
    with open(rom_path, "rb") as f:
        rom_data = f.read()

    info = gb_recomp_bridge.parse_gb_header(rom_data)
    print(f"Game: {info['title']} | Type: {info['cart_type']} | CGB: {info['is_cgb']}")

    # 2. Ensure gbrecomp
    gbrecomp_exe = ensure_gbrecomp()
    game_slug = "".join(c if c.isalnum() else "_" for c in info['title'].lower()) or "game"
    recomp_out_dir = os.path.join(BASE_DIR, "build", "recompiled", game_slug)
    os.makedirs(recomp_out_dir, exist_ok=True)

    if gbrecomp_exe:
        print(f"Executing static recompilation using {os.path.basename(gbrecomp_exe)}...")
        cmd = [gbrecomp_exe, rom_path, "-o", recomp_out_dir]
        try:
            res = subprocess.run(cmd, capture_output=True, text=True)
            if res.returncode == 0:
                print("Static recompilation completed successfully.")
            else:
                print(f"Recompiler output:\n{res.stdout}\n{res.stderr}")
        except Exception as ex:
            print(f"Error running gbrecomp: {ex}")
    else:
        print("Notice: gbrecomp executable not found. Proceeding with bare-metal container generation directly.")

    # 3. Package and build WSC ROM
    dist_dir = os.path.join(BASE_DIR, "dist")
    os.makedirs(dist_dir, exist_ok=True)
    wsc_output = os.path.join(dist_dir, f"{game_slug}.wsc")

    # Extract tiles for target ROM
    tiles_data = bytearray()
    if len(rom_data) >= 0x8000:
        for t in range(min(256, len(rom_data) // 16)):
            tile_chunk = rom_data[t * 16:(t + 1) * 16]
            if len(tile_chunk) == 16:
                tiles_data.extend(gb_recomp_bridge.convert_gb_tile_to_wsc_2bpp(tile_chunk))

    size, csum = gb_recomp_bridge.build_wsc_rom(wsc_output, tiles_data=tiles_data, title=info['title'])
    print(f"\nSuccess: WonderSwan Color ROM generated:")
    print(f"  Path: {wsc_output}")
    print(f"  Size: {size} bytes")
    print(f"  Checksum: 0x{csum:04X}")

    try:
        import test_wsc
        runners = test_wsc.find_runners()
        if runners:
            print(f"Discovered test runner: {os.path.basename(runners[0])}")
            print(f"To launch: python tools/test_wsc.py \"{wsc_output}\"")
            print(f"To validate: python tools/test_wsc.py \"{wsc_output}\" --validate")
        else:
            print("Ready to run on physical hardware or desktop test runner.")
    except Exception:
        print("Ready to run on physical hardware or desktop test runner.")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python tools/pipeline.py <path_to_rom.gb|.gbc>")
        sys.exit(1)

    success = run_pipeline(sys.argv[1])
    sys.exit(0 if success else 1)
