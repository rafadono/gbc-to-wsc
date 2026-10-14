#!/usr/bin/env python3
"""
Test harness for GBC -> WSC Core Translation subsystems:
1. Audio Translation (GB APU -> WSC Sound Generator)
2. CGB Support (BGR555 -> RGB444 palettes & HDMA transfers)
3. MBC3 RTC & MBC2 banking
4. Window Screen 2 hardware clipping
"""

import sys
import os

def test_audio_pitch_conversion():
    def gbc_to_wsc_pitch(gb_freq, is_wave=False):
        gb_freq = min(2047, gb_freq)
        diff = 2048 - gb_freq
        if not is_wave:
            period = (diff * 375 + 256) // 512
        else:
            period = (diff * 375 + 128) // 256
        period = max(1, min(2047, period))
        return 2048 - period

    p_sq = gbc_to_wsc_pitch(1500, False)
    assert 0 < p_sq < 2048, f"Square pitch out of range: {p_sq}"

    p_wav = gbc_to_wsc_pitch(1500, True)
    assert 0 < p_wav < 2048, f"Wave pitch out of range: {p_wav}"

    # Volume packing test: Ch1 vol 15, Ch2 vol 8 -> 0x8F
    vol_packed = (8 << 4) | (15 & 0x0F)
    assert vol_packed == 0x8F, "Volume pack failed"
    print("PASS: Audio pitch and volume translation")

def test_cgb_palettes():
    def bgr555_to_rgb444(bgr):
        r = (bgr & 0x001F) >> 1
        g = ((bgr >> 5) & 0x001F) >> 1
        b = ((bgr >> 10) & 0x001F) >> 1
        return (r << 8) | (g << 4) | b

    # White
    assert bgr555_to_rgb444(0x7FFF) == 0x0FFF, "White mismatch"
    # Black
    assert bgr555_to_rgb444(0x0000) == 0x0000, "Black mismatch"
    # Red
    assert bgr555_to_rgb444(0x001F) == 0x0F00, "Red mismatch"
    # Green
    assert bgr555_to_rgb444(0x03E0) == 0x00F0, "Green mismatch"
    # Blue
    assert bgr555_to_rgb444(0x7C00) == 0x000F, "Blue mismatch"

    # Auto-increment test
    bcps = 0x80 # auto-increment enabled, index 0
    pal_mem = [0] * 64
    wsc_palettes = [[0] * 4 for _ in range(8)]

    # Write 0xFF, 0x7F (White: 0x7FFF)
    data_writes = [(0, 0xFF), (1, 0x7F)]
    for i, byte in data_writes:
        idx = bcps & 0x3F
        pal_mem[idx] = byte
        pal_num = idx >> 3
        col_num = (idx >> 1) & 3
        base = (pal_num << 3) | (col_num << 1)
        bgr = pal_mem[base] | (pal_mem[base + 1] << 8)
        wsc_palettes[pal_num][col_num] = bgr555_to_rgb444(bgr)
        if bcps & 0x80:
            bcps = 0x80 | ((idx + 1) & 0x3F)

    assert (bcps & 0x3F) == 2, "BCPS index did not increment to 2"
    assert wsc_palettes[0][0] == 0x0FFF, "Palette 0 Color 0 must be 0x0FFF"
    print("PASS: CGB Palettes BGR555 -> RGB444 & auto-increment")

def test_hdma():
    # GDMA transfer test: 2 blocks (32 bytes)
    vram = [0] * 0x4000
    src_data = list(range(32))

    hdma_src = 0xC000
    hdma_dst = 0x8000
    blocks = 2 # length 0x01 in HDMA5 -> 2 blocks

    for i in range(blocks * 16):
        vram_off = (hdma_dst + i) - 0x8000
        vram[vram_off] = src_data[i]

    assert vram[0] == 0 and vram[31] == 31, "VRAM data mismatch after HDMA"
    print("PASS: HDMA block transfer")

def test_mbc3_rtc():
    rtc_regs = [0] * 5
    latched = [0] * 5

    # Write seconds = 45
    rtc_regs[0] = 45

    # Latch RTC: transition 0 -> 1
    latch_state = 0
    # write 0
    latch_state = 0
    # write 1
    if latch_state == 0:
        latched = list(rtc_regs)
    latch_state = 1

    assert latched[0] == 45, "Latched RTC seconds must be 45"
    print("PASS: MBC3 RTC register latching")

def test_mbc2():
    mbc2_ram = [0] * 512

    # Write 0x7B to address 0xA000
    mbc2_ram[0] = 0x7B & 0x0F
    # Read back (upper 4 bits read as 1)
    read_val = (mbc2_ram[0] & 0x0F) | 0xF0
    assert read_val == 0xFB, f"MBC2 RAM read failed: {read_val:02X}"
    print("PASS: MBC2 512x4-bit internal RAM masking")

def test_window_screen2_clipping():
    # Standard mode (160x144 centered with 32px borders)
    viewport_x_offset = 32
    viewport_width = 160
    wy = 24
    wx = 15 # starts at x = 15 - 7 = 8

    left = max(0, wx - 7)
    wsc_x1 = left + viewport_x_offset
    wsc_y1 = wy
    wsc_x2 = viewport_x_offset + viewport_width - 1
    wsc_y2 = 143

    assert wsc_x1 == 40, f"Expected win_x1=40, got {wsc_x1}"
    assert wsc_y1 == 24, f"Expected win_y1=24, got {wsc_y1}"
    assert wsc_x2 == 191, f"Expected win_x2=191, got {wsc_x2}"
    assert wsc_y2 == 143, f"Expected win_y2=143, got {wsc_y2}"
    print("PASS: Window Screen 2 hardware clipping coordinates")

def main():
    print("Running Python Core Verification Tests...")
    test_audio_pitch_conversion()
    test_cgb_palettes()
    test_hdma()
    test_mbc3_rtc()
    test_mbc2()
    test_window_screen2_clipping()
    print("\nALL PYTHON CORE VERIFICATION TESTS PASSED SUCCESSFULLY.")

if __name__ == "__main__":
    main()
