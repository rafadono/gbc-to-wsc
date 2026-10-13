#!/usr/bin/env python3
"""
GB to WonderSwan Color Compatibility Layer Bridge Tool
Extracts and converts Game Boy / Game Boy Color ROM structures,
validates tile transposition, and generates C header/source stubs.
"""

import sys
import os
import struct

def parse_gb_header(rom_bytes):
    if len(rom_bytes) < 0x150:
        raise ValueError("ROM file is too small to contain a valid header.")

    title_bytes = rom_bytes[0x0134:0x0143]
    title = title_bytes.split(b'\x00')[0].decode('ascii', errors='ignore').strip()
    cgb_flag = rom_bytes[0x0143]
    cart_type = rom_bytes[0x0147]
    rom_size_code = rom_bytes[0x0148]
    ram_size_code = rom_bytes[0x0149]

    rom_sizes = {
        0x00: 32 * 1024,
        0x01: 64 * 1024,
        0x02: 128 * 1024,
        0x03: 256 * 1024,
        0x04: 512 * 1024,
        0x05: 1024 * 1024,
        0x06: 2048 * 1024,
        0x07: 4096 * 1024,
        0x08: 8192 * 1024,
    }

    ram_sizes = {
        0x00: 0,
        0x01: 2 * 1024,
        0x02: 8 * 1024,
        0x03: 32 * 1024,
        0x04: 128 * 1024,
        0x05: 64 * 1024,
    }

    mbc_types = {
        0x00: "ROM ONLY",
        0x01: "MBC1",
        0x02: "MBC1+RAM",
        0x03: "MBC1+RAM+BATTERY",
        0x05: "MBC2",
        0x06: "MBC2+BATTERY",
        0x0F: "MBC3+TIMER+BATTERY",
        0x10: "MBC3+TIMER+RAM+BATTERY",
        0x11: "MBC3",
        0x12: "MBC3+RAM",
        0x13: "MBC3+RAM+BATTERY",
        0x19: "MBC5",
        0x1A: "MBC5+RAM",
        0x1B: "MBC5+RAM+BATTERY",
        0x1C: "MBC5+RUMBLE",
        0x1D: "MBC5+RUMBLE+RAM",
        0x1E: "MBC5+RUMBLE+RAM+BATTERY",
    }

    return {
        "title": title,
        "is_cgb": (cgb_flag in (0x80, 0xC0)),
        "cart_type": mbc_types.get(cart_type, f"Unknown (0x{cart_type:02X})"),
        "rom_size": rom_sizes.get(rom_size_code, len(rom_bytes)),
        "ram_size": ram_sizes.get(ram_size_code, 0),
    }

def convert_gb_tile_to_wsc_2bpp(gb_tile_16b):
    if len(gb_tile_16b) != 16:
        raise ValueError("Tile must be exactly 16 bytes")

    wsc_tile = bytearray(16)
    for y in range(8):
        wsc_tile[y] = gb_tile_16b[y * 2]
        wsc_tile[y + 8] = gb_tile_16b[y * 2 + 1]
    return bytes(wsc_tile)

def convert_bgr555_to_rgb444(bgr555):
    r = (bgr555 & 0x001F) >> 1
    g = ((bgr555 >> 5) & 0x001F) >> 1
    b = ((bgr555 >> 10) & 0x001F) >> 1
    return (r << 8) | (g << 4) | b

def test_internal_conversion():
    test_tile = bytes([
        0x3C, 0x7E,
        0x42, 0x42,
        0x42, 0x42,
        0x42, 0x42,
        0x7E, 0x3C,
        0x42, 0x42,
        0x42, 0x42,
        0x3C, 0x7E
    ])
    wsc = convert_gb_tile_to_wsc_2bpp(test_tile)
    assert wsc[0] == 0x3C
    assert wsc[8] == 0x7E
    assert convert_bgr555_to_rgb444(0x7FFF) == 0x0FFF
    assert convert_bgr555_to_rgb444(0x0000) == 0x0000
    print("Bridge internal test passed.")

def build_wsc_rom(output_path, tiles_data=None, title="GB-TO-WSC"):
    rom_size = 128 * 1024
    rom = bytearray(rom_size)

    # 1. V30MZ Startup Machine Code at offset 0x2000 (Physical address 0x22000 or 0x02000)
    # CLI; CLD; XOR AX, AX; MOV DS, AX; MOV ES, AX; MOV SS, AX; MOV SP, 0x3FFE
    code = bytearray([
        0xFA,                         # CLI
        0xFC,                         # CLD
        0x31, 0xC0,                   # XOR AX, AX
        0x8E, 0xD8,                   # MOV DS, AX
        0x8E, 0xC0,                   # MOV ES, AX
        0x8E, 0xD0,                   # MOV SS, AX
        0xBC, 0xFE, 0x3F,             # MOV SP, 0x3FFE

        # Switch to Color mode: OUT 0x60, AL (AL=0x80)
        0xB0, 0x80,                   # MOV AL, 0x80
        0xBA, 0x60, 0x00,             # MOV DX, 0x0060
        0xEE,                         # OUT DX, AL

        # Enable Screen 1 + Sprites: OUT 0x00, AX (AX=0x0005)
        0xB8, 0x05, 0x00,             # MOV AX, 0x0005
        0xBA, 0x00, 0x00,             # MOV DX, 0x0000
        0xEF,                         # OUT DX, AX

        # Loop: HLT; JMP -3
        0xF4,                         # HLT
        0xEB, 0xFD                    # JMP short to HLT
    ])

    entry_offset = 0x2000
    rom[entry_offset:entry_offset + len(code)] = code

    # 2. Embed translated tiles if provided
    if tiles_data:
        # Place tiles in ROM data bank at 0x4000
        tiles_len = min(len(tiles_data), 0x2000)
        rom[0x4000:0x4000 + tiles_len] = tiles_data[:tiles_len]

    # 3. CPU Reset Vector at ROM end - 16 (0x1FFF0 in 128KB ROM)
    # Far JUMP to 0x2000:0x2000 (EA 00 20 00 20)
    reset_vector_offset = rom_size - 16
    rom[reset_vector_offset:reset_vector_offset + 5] = bytearray([
        0xEA, 0x00, 0x20, 0x00, 0x00  # JMP FAR 0000:2000
    ])

    # 4. WonderSwan Color Cartridge Header (last 10 bytes: 0x1FFF6 - 0x1FFFF)
    header_offset = rom_size - 10
    rom[header_offset + 0] = 0x00     # Developer ID
    rom[header_offset + 1] = 0x01     # Minimum console: 0x01 = WonderSwan Color
    rom[header_offset + 2] = 0x42     # Cartridge ID
    rom[header_offset + 3] = 0x00     # Reserved
    rom[header_offset + 4] = 0x01     # ROM Size: 0x01 = 1 Mbit (128 KB)
    rom[header_offset + 5] = 0x00     # Save RAM: None
    rom[header_offset + 6] = 0x04     # Flags: Horizontal orientation
    rom[header_offset + 7] = 0x00     # RTC: None

    # 5. Compute 16-bit additive checksum for bytes 0 to (rom_size - 3)
    checksum = sum(rom[:rom_size - 2]) & 0xFFFF
    rom[rom_size - 2] = checksum & 0xFF
    rom[rom_size - 1] = (checksum >> 8) & 0xFF

    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(rom)

    return len(rom), checksum

if __name__ == "__main__":
    test_internal_conversion()

    # Generate a sample verified WSC ROM
    sample_tiles = bytearray()
    test_tile = bytes([
        0x3C, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x7E, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x7E
    ])
    sample_tiles.extend(convert_gb_tile_to_wsc_2bpp(test_tile))

    wsc_path = os.path.join("build", "gbc_bridge_sample.wsc")
    size, csum = build_wsc_rom(wsc_path, tiles_data=sample_tiles)
    print(f"Generated WonderSwan Color ROM: {wsc_path} ({size} bytes, Checksum: 0x{csum:04X})")

    if len(sys.argv) > 1:
        rom_path = sys.argv[1]
        if os.path.exists(rom_path):
            with open(rom_path, "rb") as f:
                data = f.read()
            info = parse_gb_header(data)
            print("ROM Header Info:")
            for k, v in info.items():
                print(f"  {k}: {v}")

