#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gbc_hal.h"

static int s_tests_passed = 0;
static int s_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        s_tests_passed++; \
    } else { \
        s_tests_failed++; \
        printf("[FAIL] Line %d: %s\n", __LINE__, msg); \
    } \
} while(0)

static void test_tile_conversion_2bpp(void) {
    uint8_t gb_tile[16] = {
        0x3C, 0x7E,
        0x42, 0x42,
        0x42, 0x42,
        0x42, 0x42,
        0x7E, 0x3C,
        0x42, 0x42,
        0x42, 0x42,
        0x3C, 0x7E
    };

    uint8_t wsc_tile[16];
    memset(wsc_tile, 0, sizeof(wsc_tile));

    gbc_tile_to_wsc_2bpp(gb_tile, wsc_tile);

    TEST_ASSERT(wsc_tile[0] == 0x3C, "Tile row 0 plane 0 must match");
    TEST_ASSERT(wsc_tile[8] == 0x7E, "Tile row 0 plane 1 must match");
    TEST_ASSERT(wsc_tile[1] == 0x42, "Tile row 1 plane 0 must match");
    TEST_ASSERT(wsc_tile[9] == 0x42, "Tile row 1 plane 1 must match");
    TEST_ASSERT(wsc_tile[4] == 0x7E, "Tile row 4 plane 0 must match");
    TEST_ASSERT(wsc_tile[12] == 0x3C, "Tile row 4 plane 1 must match");
}

static void test_color_conversion(void) {
    uint16_t white_bgr = 0x7FFF;
    uint16_t white_wsc = gbc_to_wsc_color(white_bgr);
    TEST_ASSERT(white_wsc == 0x0FFF, "White BGR555 -> RGB444 must be 0x0FFF");

    uint16_t black_bgr = 0x0000;
    uint16_t black_wsc = gbc_to_wsc_color(black_bgr);
    TEST_ASSERT(black_wsc == 0x0000, "Black BGR555 -> RGB444 must be 0x0000");

    uint16_t pure_red_bgr = 0x001F;
    uint16_t red_wsc = gbc_to_wsc_color(pure_red_bgr);
    TEST_ASSERT(red_wsc == 0x0F00, "Red BGR555 -> RGB444 must be 0x0F00");
}

static void test_memory_and_banking(void) {
    uint8_t dummy_rom[0x8000];
    memset(dummy_rom, 0xAA, 0x4000);
    memset(dummy_rom + 0x4000, 0xBB, 0x4000);

    gbc_hal_init(dummy_rom, sizeof(dummy_rom), NULL, 0);

    TEST_ASSERT(gbc_read8(0x0000) == 0xAA, "ROM bank 0 read must match");
    TEST_ASSERT(gbc_read8(0x4000) == 0xBB, "ROM bank 1 read must match");

    gbc_write8(0xC000, 0x42);
    TEST_ASSERT(gbc_read8(0xC000) == 0x42, "WRAM write/read must match");

    gbc_write8(0x8000, 0x55);
    TEST_ASSERT(gbc_read8(0x8000) == 0x55, "VRAM write/read must match");
    TEST_ASSERT(g_video.tile_dirty[0] == 1, "VRAM write must mark tile 0 dirty");

    gbc_write8(0xFF80, 0x99);
    TEST_ASSERT(gbc_read8(0xFF80) == 0x99, "HRAM write/read must match");

    gbc_write8(0xFFFF, 0x1F);
    TEST_ASSERT(gbc_read8(0xFFFF) == 0x1F, "IE register write/read must match");
}

static void test_dma_transfer(void) {
    for (int i = 0; i < 0xA0; i++) {
        gbc_write8(0xC000 + i, (uint8_t)i);
    }

    gbc_write8(GB_REG_DMA, 0xC0);

    bool dma_ok = true;
    for (int i = 0; i < 0xA0; i++) {
        if (gbc_read8(0xFE00 + i) != (uint8_t)i) {
            dma_ok = false;
            break;
        }
    }
    TEST_ASSERT(dma_ok, "OAM DMA transfer must copy all 160 bytes");
}

static void test_input_translation(void) {
    gbc_input_init();

    gbc_input_set_button(GB_BTN_RIGHT, true);
    gbc_input_set_button(GB_BTN_A, true);

    uint8_t p1_dirs = gbc_input_read_p1(0x20);
    TEST_ASSERT((p1_dirs & 0x01) == 0, "Right button must read as active-low 0");
    TEST_ASSERT((p1_dirs & 0x02) != 0, "Left button must read as unpressed 1");

    uint8_t p1_actions = gbc_input_read_p1(0x10);
    TEST_ASSERT((p1_actions & 0x01) == 0, "A button must read as active-low 0");
    TEST_ASSERT((p1_actions & 0x02) != 0, "B button must read as unpressed 1");
}

static void test_timers_and_interrupts(void) {
    gbc_hal_reset();

    gbc_write8(GB_REG_TMA, 0xF0);
    gbc_write8(GB_REG_TIMA, 0xFE);
    gbc_write8(GB_REG_TAC, 0x05);

    gbc_hal_step_timers(16);
    TEST_ASSERT(gbc_read8(GB_REG_TIMA) == 0xFF, "TIMA should increment to 0xFF");

    gbc_hal_step_timers(16);
    TEST_ASSERT(gbc_read8(GB_REG_TIMA) == 0xF0, "TIMA should reload TMA 0xF0 on overflow");
    TEST_ASSERT((g_gb.if_reg & GB_INT_TIMER) != 0, "Timer interrupt flag must be set");
}

static void test_frame_pacing(void) {
    gbc_hal_reset();

    int frames_synced = 0;
    for (int i = 0; i < 10; i++) {
        if (gbc_hal_frame_sync()) {
            frames_synced++;
        }
    }
    TEST_ASSERT(frames_synced == 8, "10 WSC frames must yield exactly 8 GB frames (4:5 ratio)");
}

static void test_sprite_translation_8x8(void) {
    gbc_hal_reset();
    g_video.lcdc |= (GB_LCDC_ENABLE | GB_LCDC_OBJ_ENABLE);
    g_video.lcdc &= ~GB_LCDC_OBJ_SIZE; // 8x8 mode

    // Setup 1 sprite in OAM: Y=32 (actual y=16), X=16 (actual x=8), tile=0x05, flags=0x20 (H-flip)
    g_gb.oam[0] = 32;
    g_gb.oam[1] = 16;
    g_gb.oam[2] = 0x05;
    g_gb.oam[3] = 0x20;

    gbc_video_render_frame();

    TEST_ASSERT(g_video.wsc_sprite_count == 1, "Should generate exactly 1 WSC sprite in 8x8 mode");
    TEST_ASSERT(g_video.wsc_sprites[0].tile == 0x05, "Sprite tile should match");
    TEST_ASSERT(g_video.wsc_sprites[0].x == (8 + GB_VIEWPORT_X_OFFSET), "Sprite X coordinate must include viewport offset (40)");
    TEST_ASSERT(g_video.wsc_sprites[0].y == 16, "Sprite Y coordinate must be 16");
    TEST_ASSERT((g_video.wsc_sprites[0].flags & WSC_SPR_ATTR_HFLIP) != 0, "Horizontal flip flag must be set");
}

static void test_sprite_translation_8x16(void) {
    gbc_hal_reset();
    g_video.lcdc |= (GB_LCDC_ENABLE | GB_LCDC_OBJ_ENABLE | GB_LCDC_OBJ_SIZE); // 8x16 mode

    // Setup 1 sprite in OAM: Y=32, X=16, tile=0x04, flags=0x00
    g_gb.oam[0] = 32;
    g_gb.oam[1] = 16;
    g_gb.oam[2] = 0x04;
    g_gb.oam[3] = 0x00;

    gbc_video_render_frame();

    TEST_ASSERT(g_video.wsc_sprite_count == 2, "8x16 mode must split into two WSC 8x8 sprites");
    TEST_ASSERT(g_video.wsc_sprites[0].tile == 0x04, "Top sprite tile must be 0x04");
    TEST_ASSERT(g_video.wsc_sprites[0].y == 16, "Top sprite Y must be 16");
    TEST_ASSERT(g_video.wsc_sprites[1].tile == 0x05, "Bottom sprite tile must be 0x05");
    TEST_ASSERT(g_video.wsc_sprites[1].y == 24, "Bottom sprite Y must be 24 (16+8)");
}

static void test_window_layer(void) {
    gbc_hal_reset();
    g_video.lcdc |= (GB_LCDC_ENABLE | GB_LCDC_WIN_ENABLE);
    g_video.wx = 10;
    g_video.wy = 20;

    gbc_video_render_frame();

    TEST_ASSERT(g_video.window_enabled == true, "Window should be enabled");
    TEST_ASSERT(g_video.win_y == 20, "Window Y should match WY");
    TEST_ASSERT(g_video.win_x == (3 + GB_VIEWPORT_X_OFFSET), "Window X should map WX-7 + viewport offset (35)");
}

static void test_mbc1_banking(void) {
    uint8_t mbc1_rom[0x40000]; // 256 KB (16 banks)
    memset(mbc1_rom, 0, sizeof(mbc1_rom));
    mbc1_rom[0x0147] = 0x01; // MBC1

    for (int b = 0; b < 16; b++) {
        memset(mbc1_rom + (b * 0x4000), (uint8_t)b, 0x4000);
    }
    mbc1_rom[0x0147] = 0x01; // Re-set header after fill

    gbc_hal_init(mbc1_rom, sizeof(mbc1_rom), NULL, 0);

    TEST_ASSERT(g_gb.mbc_type == MBC1, "Cartridge must be identified as MBC1");
    TEST_ASSERT(gbc_read8(0x0000) == 0x00, "Bank 0 should read from 0x0000");

    // Select bank 5
    gbc_write8(0x2000, 0x05);
    TEST_ASSERT(gbc_read8(0x4000) == 0x05, "Switchable ROM bank 0x4000 should read bank 5");

    // Selecting bank 0 should automatically correct to bank 1 in MBC1
    gbc_write8(0x2000, 0x00);
    TEST_ASSERT(gbc_read8(0x4000) == 0x01, "Selecting bank 0 in MBC1 must resolve to bank 1");

    // Test upper 2 bits (bank 2 | (1 << 5) = bank 34, modulo 16 banks = bank 2)
    gbc_write8(0x4000, 0x01); // High bits = 1 (bank 32)
    gbc_write8(0x2000, 0x03); // Low bits = 3 -> bank 35 % 16 = bank 3
    TEST_ASSERT(gbc_read8(0x4000) == 0x03, "High bank bits should mirror correctly across ROM size");
}

static void test_mbc5_banking(void) {
    uint8_t mbc5_rom[0x40000]; // 256 KB (16 banks)
    memset(mbc5_rom, 0, sizeof(mbc5_rom));
    mbc5_rom[0x0147] = 0x19; // MBC5

    for (int b = 0; b < 16; b++) {
        memset(mbc5_rom + (b * 0x4000), (uint8_t)b, 0x4000);
    }
    mbc5_rom[0x0147] = 0x19;

    gbc_hal_init(mbc5_rom, sizeof(mbc5_rom), NULL, 0);

    TEST_ASSERT(g_gb.mbc_type == MBC5, "Cartridge must be identified as MBC5");

    // In MBC5, bank 0 IS valid at 0x4000-0x7FFF
    gbc_write8(0x2000, 0x00);
    gbc_write8(0x3000, 0x00);
    TEST_ASSERT(gbc_read8(0x4000) == 0x00, "MBC5 allows bank 0 in switchable slot");

    // Select bank 7
    gbc_write8(0x2000, 0x07);
    TEST_ASSERT(gbc_read8(0x4000) == 0x07, "MBC5 selects bank 7");
}

static void test_screen_mode_standard_borders(void) {
    gbc_hal_reset();
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_STANDARD);

    TEST_ASSERT(gbc_video_get_screen_mode() == GBC_SCREEN_MODE_STANDARD, "Screen mode must be standard");
    TEST_ASSERT(gbc_video_get_viewport_x_offset() == 32, "Standard mode offset must be 32 px");
    TEST_ASSERT(gbc_video_get_viewport_width() == 160, "Standard mode viewport width must be 160 px");

    g_video.lcdc |= (GB_LCDC_ENABLE | GB_LCDC_OBJ_ENABLE);
    g_video.lcdc &= ~GB_LCDC_OBJ_SIZE;

    // Sprite 0: Within 160px bounds (GB X=16, actual X=8, WSC X=40)
    g_gb.oam[0] = 32; g_gb.oam[1] = 16; g_gb.oam[2] = 0x01; g_gb.oam[3] = 0x00;

    // Sprite 1: Outside 160px bounds, spilling into right black border (GB X=170, actual X=162, WSC X=194)
    g_gb.oam[4] = 32; g_gb.oam[5] = 170; g_gb.oam[6] = 0x02; g_gb.oam[7] = 0x00;

    gbc_video_render_frame();

    TEST_ASSERT(g_video.wsc_sprite_count == 1, "Sprite in 32px black border zone must be culled in standard mode");
    TEST_ASSERT(g_video.wsc_sprites[0].tile == 0x01, "Only the inside sprite should be rendered");
}

static void test_screen_mode_widescreen(void) {
    gbc_hal_reset();
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_WIDESCREEN);

    TEST_ASSERT(gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN, "Screen mode must be widescreen");
    TEST_ASSERT(gbc_video_get_viewport_x_offset() == 0, "Widescreen mode offset must be 0 px");
    TEST_ASSERT(gbc_video_get_viewport_width() == 224, "Widescreen viewport width must be 224 px");

    g_video.lcdc |= (GB_LCDC_ENABLE | GB_LCDC_OBJ_ENABLE);
    g_video.lcdc &= ~GB_LCDC_OBJ_SIZE;

    // Sprite at GB X=16 -> WSC X = 8 (no 32px shift)
    g_gb.oam[0] = 32; g_gb.oam[1] = 16; g_gb.oam[2] = 0x03; g_gb.oam[3] = 0x00;

    // Sprite at wide coordinate (GB X=216, actual X=208) -> Visible across the 224px display
    g_gb.oam[4] = 32; g_gb.oam[5] = 216; g_gb.oam[6] = 0x04; g_gb.oam[7] = 0x00;

    gbc_video_render_frame();

    TEST_ASSERT(g_video.wsc_sprite_count == 2, "Both sprites must be visible across the full 224px widescreen");
    TEST_ASSERT(g_video.wsc_sprites[0].x == 8, "Sprite X in widescreen has 0px offset");
    TEST_ASSERT(g_video.wsc_sprites[1].x == 208, "Wide sprite X is rendered at coordinate 208");

    // Reset back to standard mode
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_STANDARD);
}

static void test_hot_toggle_screen_mode_button(void) {
    gbc_hal_reset();
    gbc_input_init();
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_STANDARD);

    TEST_ASSERT(gbc_video_get_screen_mode() == GBC_SCREEN_MODE_STANDARD, "Initial mode is standard");

    // Press Y4 button
    gbc_input_process_wsc_keys(WSC_KEY_Y4);
    TEST_ASSERT(gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN, "Pressing Y4 must toggle mode to widescreen");

    // Release Y4 (mode remains widescreen)
    gbc_input_process_wsc_keys(0);
    TEST_ASSERT(gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN, "Releasing Y4 preserves widescreen mode");

    // Press Y4 again
    gbc_input_process_wsc_keys(WSC_KEY_Y4);
    TEST_ASSERT(gbc_video_get_screen_mode() == GBC_SCREEN_MODE_STANDARD, "Pressing Y4 again toggles back to standard");
}

static void test_audio_translation(void) {
    gbc_audio_reset();

    uint16_t p_sq = gbc_to_wsc_pitch(1500, false);
    TEST_ASSERT(p_sq > 0 && p_sq < 2048, "Square channel pitch must be within 0-2047");

    uint16_t p_wav = gbc_to_wsc_pitch(1500, true);
    TEST_ASSERT(p_wav > 0 && p_wav < 2048, "Wave channel pitch must be within 0-2047");

    // Test channel 1 volume and trigger
    gbc_write8(GB_REG_NR12, 0xF2); // vol 15
    gbc_write8(GB_REG_NR13, 0x00);
    gbc_write8(GB_REG_NR14, 0x87); // trigger + freq 7
    TEST_ASSERT(g_audio.ch1_active == true, "Ch1 must be active after trigger");
    TEST_ASSERT(g_audio.ch1_vol == 15, "Ch1 vol must be 15");

    // Test channel 2 volume and trigger
    gbc_write8(GB_REG_NR22, 0x82); // vol 8
    gbc_write8(GB_REG_NR24, 0x87); // trigger
    TEST_ASSERT((g_audio.wsc_vol_ch1_ch2 & 0x0F) == 15, "WSC Ch1 volume nibble must be 15");
    TEST_ASSERT(((g_audio.wsc_vol_ch1_ch2 >> 4) & 0x0F) == 8, "WSC Ch2 volume nibble must be 8");

    // Test Wave RAM write and readback
    gbc_write8(GB_REG_WAVE_START + 0, 0xAB);
    gbc_write8(GB_REG_WAVE_START + 15, 0x12);
    TEST_ASSERT(gbc_read8(GB_REG_WAVE_START + 0) == 0xAB, "Wave RAM byte 0 readback must match");
    TEST_ASSERT(gbc_read8(GB_REG_WAVE_START + 15) == 0x12, "Wave RAM byte 15 readback must match");

    // Test Master volume and panning
    gbc_write8(GB_REG_NR50, 0x77);
    gbc_write8(GB_REG_NR51, 0xFF);
    TEST_ASSERT(g_audio.wsc_output_mode == 0xFF, "WSC audio output mode must match NR51");
}

static void test_cgb_palettes(void) {
    gbc_video_reset();

    // Auto-increment bit 7 set, start at index 0
    gbc_write8(GB_REG_BCPS, 0x80);
    // Write color 0 (white: 0x7FFF)
    gbc_write8(GB_REG_BCPD, 0xFF);
    gbc_write8(GB_REG_BCPD, 0x7F);

    TEST_ASSERT(g_video.wsc_bg_palettes[0][0] == 0x0FFF, "CGB Palette 0 Color 0 must convert to 0x0FFF (white)");
    TEST_ASSERT((gbc_read8(GB_REG_BCPS) & 0x3F) == 2, "BCPS index must auto-increment to 2");

    // Write OBJ palette 0 Color 1 (red: 0x001F)
    gbc_write8(GB_REG_OCPS, 0x80 | 0x02);
    gbc_write8(GB_REG_OCPD, 0x1F);
    gbc_write8(GB_REG_OCPD, 0x00);
    TEST_ASSERT(g_video.wsc_obj_palettes[0][1] == 0x0F00, "CGB OBJ Palette 0 Color 1 must convert to 0x0F00 (red)");
}

static void test_hdma_transfers(void) {
    gbc_hal_reset();
    g_gb.is_cgb = true;

    // Place test pattern in WRAM at 0xC100
    for (int i = 0; i < 32; i++) {
        gbc_write8(0xC100 + i, (uint8_t)(0xAA + i));
    }

    // Configure HDMA: source 0xC100, destination 0x8000
    gbc_write8(GB_REG_HDMA1, 0xC1);
    gbc_write8(GB_REG_HDMA2, 0x00);
    gbc_write8(GB_REG_HDMA3, 0x00);
    gbc_write8(GB_REG_HDMA4, 0x00);

    // General purpose DMA: 2 blocks (32 bytes), bit 7 = 0
    gbc_write8(GB_REG_HDMA5, 0x01);

    TEST_ASSERT(gbc_read8(0x8000) == 0xAA, "VRAM byte 0 after GDMA must match source");
    TEST_ASSERT(gbc_read8(0x801F) == (uint8_t)(0xAA + 31), "VRAM byte 31 after GDMA must match source");
    TEST_ASSERT(gbc_read8(GB_REG_HDMA5) == 0xFF, "HDMA5 must read 0xFF after GDMA completion");
}

static void test_mbc3_banking_and_rtc(void) {
    uint8_t test_rom[0x4000 * 8];
    memset(test_rom, 0, sizeof(test_rom));
    for (int b = 0; b < 8; b++) {
        test_rom[b * 0x4000] = (uint8_t)(0x30 + b);
    }
    uint8_t test_sram[0x2000 * 4];
    memset(test_sram, 0, sizeof(test_sram));

    gbc_memory_init(&g_gb, test_rom, sizeof(test_rom), test_sram, sizeof(test_sram));
    g_gb.mbc_type = MBC3;

    // Enable RAM & RTC
    gbc_write8(0x0000, 0x0A);
    TEST_ASSERT(g_gb.sram_enabled == true, "MBC3 RAM & RTC must be enabled with 0x0A");

    // Select ROM bank 4
    gbc_write8(0x2000, 0x04);
    TEST_ASSERT(g_gb.current_rom_bank == 4, "MBC3 ROM bank 4 must be selected");
    TEST_ASSERT(gbc_read8(0x4000) == 0x34, "Read at 0x4000 must match bank 4");

    // Select RTC Register 0x08 (seconds)
    gbc_write8(0x4000, 0x08);
    TEST_ASSERT(g_gb.mbc3_rtc_mapped == true, "RTC register mapping must be active");
    gbc_write8(0xA000, 42); // 42 seconds
    TEST_ASSERT(g_gb.mbc3_rtc_regs[0] == 42, "RTC seconds register must hold written value");

    // Latch RTC: write 0 then 1 to 0x6000
    gbc_write8(0x6000, 0x00);
    gbc_write8(0x6000, 0x01);
    TEST_ASSERT(gbc_read8(0xA000) == 42, "Latched RTC seconds register must read 42");
}

static void test_mbc2_banking_and_ram(void) {
    uint8_t test_rom[0x4000 * 4];
    memset(test_rom, 0, sizeof(test_rom));
    for (int b = 0; b < 4; b++) {
        test_rom[b * 0x4000] = (uint8_t)(0x20 + b);
    }

    gbc_memory_init(&g_gb, test_rom, sizeof(test_rom), NULL, 0);
    g_gb.mbc_type = MBC2;

    // Address bit 8 = 0: Enable RAM
    gbc_write8(0x0000, 0x0A);
    TEST_ASSERT(g_gb.sram_enabled == true, "MBC2 RAM must be enabled");

    // Address bit 8 = 1: ROM bank select
    gbc_write8(0x0100, 0x03);
    TEST_ASSERT(g_gb.current_rom_bank == 3, "MBC2 ROM bank 3 must be selected");
    TEST_ASSERT(gbc_read8(0x4000) == 0x23, "Read at 0x4000 must match bank 3");

    // Test 512x4-bit internal RAM
    gbc_write8(0xA000, 0x7B); // lower 4 bits are 0x0B
    TEST_ASSERT(gbc_read8(0xA000) == 0xFB, "MBC2 RAM read must return upper 4 bits as 1s and lower 4 bits as stored");
}

static void test_window_screen2_clipping(void) {
    gbc_video_reset();
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_STANDARD);

    // Enable LCD & Window, set position
    g_video.lcdc = GB_LCDC_ENABLE | GB_LCDC_WIN_ENABLE;
    g_video.wy = 30;
    g_video.wx = 15; // window starts at x = 15 - 7 = 8

    gbc_video_update_window_screen2();

    TEST_ASSERT(g_video.window_enabled == true, "Window must be enabled");
    TEST_ASSERT((g_video.wsc_disp_ctrl & WSC_DISP_CTRL_SCR2_ON) != 0, "WSC Screen 2 must be turned ON for window");
    TEST_ASSERT((g_video.wsc_disp_ctrl & WSC_DISP_CTRL_WIN_ON) != 0, "WSC Screen 2 Window clipping must be ON");
    TEST_ASSERT(g_video.wsc_win_x1 == (32 + 8), "WSC win_x1 must match 32 + (15 - 7) = 40");
    TEST_ASSERT(g_video.wsc_win_y1 == 30, "WSC win_y1 must match WY (30)");
    TEST_ASSERT(g_video.wsc_win_x2 == (32 + 160 - 1), "WSC win_x2 must match viewport right edge (191)");
    TEST_ASSERT(g_video.wsc_win_y2 == 143, "WSC win_y2 must match bottom line (143)");
}

static void test_dmg_palette_presets_and_cycling(void) {
    gbc_video_reset();
    g_gb.is_cgb = false;
    gbc_features_set_dmg_palette_cycle(true);

    // Initial preset is pocket grayscale
    gbc_video_set_dmg_palette_preset(DMG_PALETTE_POCKET_GRAYSCALE);
    TEST_ASSERT(g_video.wsc_bg_palettes[0][0] == 0x0FFF, "Pocket color 0 must be 0x0FFF");
    TEST_ASSERT(strcmp(gbc_video_get_dmg_palette_name(DMG_PALETTE_POCKET_GRAYSCALE), "Game Boy Pocket Grayscale") == 0, "Pocket name mismatch");

    // Cycle palette via Y2 button -> advances to Game Boy Light Teal
    gbc_input_process_wsc_keys(WSC_KEY_Y2);
    TEST_ASSERT(gbc_video_get_dmg_palette_preset() == DMG_PALETTE_LIGHT_TEAL, "Cycle must advance to Game Boy Light teal");
    TEST_ASSERT(g_video.wsc_bg_palettes[0][0] == 0x0AFE, "Light teal color 0 must be 0x0AFE");

    // Cycle again -> advances to Virtual Boy Crimson
    gbc_input_process_wsc_keys(0);
    gbc_input_process_wsc_keys(WSC_KEY_Y2);
    TEST_ASSERT(gbc_video_get_dmg_palette_preset() == DMG_PALETTE_VIRTUAL_BOY, "Cycle must advance to Virtual Boy crimson");
    TEST_ASSERT(g_video.wsc_bg_palettes[0][0] == 0x0F22, "Virtual boy color 0 must be 0x0F22");

    // Test GBC Boot ROM preset direct selection
    gbc_video_set_dmg_palette_preset(DMG_PALETTE_GBC_BLUE);
    TEST_ASSERT(g_video.wsc_bg_palettes[0][2] == 0x025C, "GBC Blue color 2 must match");
    TEST_ASSERT(strcmp(gbc_video_get_dmg_palette_name(DMG_PALETTE_GBC_BLUE), "GBC Blue (Left)") == 0, "GBC Blue name mismatch");

    // Test Community preset selection
    gbc_video_set_dmg_palette_preset(DMG_PALETTE_CYBERPUNK_NEON);
    TEST_ASSERT(g_video.wsc_bg_palettes[0][0] == 0x00FF, "Cyberpunk neon color 0 must be cyan");
    TEST_ASSERT(strcmp(gbc_video_get_dmg_palette_name(DMG_PALETTE_CYBERPUNK_NEON), "Cyberpunk Neon") == 0, "Cyberpunk name mismatch");
}

static void test_tate_orientation_and_input(void) {
    gbc_hal_reset();
    gbc_features_set_tate_mode(true);

    TEST_ASSERT(gbc_video_get_orientation() == GBC_ORIENTATION_VERTICAL, "Orientation must be vertical");
    TEST_ASSERT(gbc_video_get_viewport_width() == 144, "Tate viewport width must be 144");

    // In Tate mode, Y2 is D-Pad UP
    gbc_input_process_wsc_keys(WSC_KEY_Y2);
    uint8_t p1 = gbc_input_read_p1(0x20); // read directions (bit 4 low)
    TEST_ASSERT((p1 & 0x04) == 0, "Direction UP bit must be active in Tate mode when Y2 is pressed");

    // In Tate mode, X4 is Action A
    gbc_input_process_wsc_keys(WSC_KEY_X4);
    p1 = gbc_input_read_p1(0x10); // read buttons (bit 5 low)
    TEST_ASSERT((p1 & 0x01) == 0, "Button A bit must be active in Tate mode when X4 is pressed");

    // Revert to horizontal
    gbc_features_set_tate_mode(false);
    TEST_ASSERT(gbc_video_get_orientation() == GBC_ORIENTATION_HORIZONTAL, "Reverting resets to horizontal");
}

static void test_enhanced_audio_harmonics(void) {
    gbc_audio_reset();

    // Raw mode: duty cycle 50%
    gbc_audio_set_enhanced(false);
    gbc_write8(GB_REG_NR11, 0x80); // 50% duty
    TEST_ASSERT(g_audio.wsc_ch1_wave[0] == 0, "Raw wave sample 0 must be 0");
    TEST_ASSERT(g_audio.wsc_ch1_wave[2] == 15, "Raw wave sample 2 must be 15");

    // Enhanced mode: duty cycle 50% smoothed
    gbc_audio_set_enhanced(true);
    TEST_ASSERT(gbc_audio_get_enhanced() == true, "Enhanced audio must be enabled");
    TEST_ASSERT(g_audio.wsc_ch1_wave[0] == 1, "Enhanced smoothed sample 0 must be 1 (tapered)");
    TEST_ASSERT(g_audio.wsc_ch1_wave[1] == 6, "Enhanced smoothed sample 1 must be 6 (ramp)");
    TEST_ASSERT(g_audio.wsc_ch1_wave[2] == 13, "Enhanced smoothed sample 2 must be 13 (band-limited)");
}

static void test_feature_disabling(void) {
    gbc_hal_reset();

    // 1. Disable widescreen toggle
    gbc_features_set_widescreen_toggle(false);
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_STANDARD);

    gbc_input_process_wsc_keys(WSC_KEY_Y4);
    TEST_ASSERT(gbc_video_get_screen_mode() == GBC_SCREEN_MODE_STANDARD, "Disabled widescreen toggle must keep standard mode");

    // 2. Disable palette cycling
    gbc_features_set_dmg_palette_cycle(false);
    gbc_video_set_dmg_palette_preset(DMG_PALETTE_ORIGINAL_GREEN);

    gbc_input_process_wsc_keys(0);
    gbc_input_process_wsc_keys(WSC_KEY_Y2);
    TEST_ASSERT(gbc_video_get_dmg_palette_preset() == DMG_PALETTE_ORIGINAL_GREEN, "Disabled palette cycle must not change preset");
}

static void test_smooth_camera_interpolation(void) {
    gbc_hal_reset();

    // 1. Disabled mode
    gbc_features_set_smooth_camera(false);
    TEST_ASSERT(!gbc_video_get_smooth_camera(), "Smooth camera must be disabled initially");

    g_video.scx = 10;
    gbc_video_update_camera_interpolation(false);
    TEST_ASSERT(gbc_video_get_render_scx() == 10, "Render SCX must match SCX when disabled");

    gbc_video_update_camera_interpolation(true);
    TEST_ASSERT(gbc_video_get_render_scx() == 10, "Held frame must not interpolate when disabled");

    // 2. Enabled mode: smooth scrolling (dx = +4)
    gbc_features_set_smooth_camera(true);
    TEST_ASSERT(gbc_video_get_smooth_camera(), "Smooth camera must be enabled");

    g_video.scx = 10;
    gbc_video_update_camera_interpolation(false);

    g_video.scx = 14;
    gbc_video_update_camera_interpolation(false);
    TEST_ASSERT(gbc_video_get_render_scx() == 14, "Render SCX must be 14 on active frame");

    // Held frame (5th frame in 4:5 cadence)
    gbc_video_update_camera_interpolation(true);
    TEST_ASSERT(gbc_video_get_render_scx() == 16, "Held frame must interpolate forward to 16 (14 + 4/2)");

    // 3. Teleport protection (large jump > 16 px)
    g_video.scx = 20;
    gbc_video_update_camera_interpolation(false);

    g_video.scx = 120;
    gbc_video_update_camera_interpolation(false);

    gbc_video_update_camera_interpolation(true);
    TEST_ASSERT(gbc_video_get_render_scx() == 120, "Teleport jump (>16) must clamp to curr_scx without interpolation");

    // 4. Disable again
    gbc_features_set_smooth_camera(false);
    TEST_ASSERT(gbc_video_get_render_scx() == 120, "Disabling must immediately revert to raw SCX");
}

int main(void) {
    printf("========================================\n");
    printf("   GBC -> WSC COMPATIBILITY LAYER TESTS \n");
    printf("========================================\n");

    test_tile_conversion_2bpp();
    test_color_conversion();
    test_memory_and_banking();
    test_dma_transfer();
    test_input_translation();
    test_timers_and_interrupts();
    test_frame_pacing();

    // Hito 2 & Hito 4 core tests:
    test_sprite_translation_8x8();
    test_sprite_translation_8x16();
    test_window_layer();
    test_mbc1_banking();
    test_mbc5_banking();

    // Screen mode tests:
    test_screen_mode_standard_borders();
    test_screen_mode_widescreen();
    test_hot_toggle_screen_mode_button();

    // Subsystems Tests:
    test_audio_translation();
    test_cgb_palettes();
    test_hdma_transfers();
    test_mbc3_banking_and_rtc();
    test_mbc2_banking_and_ram();
    test_window_screen2_clipping();

    // New Features & Disabling Tests:
    test_dmg_palette_presets_and_cycling();
    test_tate_orientation_and_input();
    test_enhanced_audio_harmonics();
    test_feature_disabling();
    test_smooth_camera_interpolation();

    printf("\nResults:\n");
    printf("  Passed: %d\n", s_tests_passed);
    printf("  Failed: %d\n", s_tests_failed);

    if (s_tests_failed == 0) {
        printf("\nALL TESTS PASSED SUCCESSFULLY (Audio, CGB, Mappers, Palettes, Tate, Enhancements verified).\n");
        return 0;
    } else {
        printf("\nTEST SUITE FAILED.\n");
        return 1;
    }
}
