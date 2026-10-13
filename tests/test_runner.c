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

    printf("\nResults:\n");
    printf("  Passed: %d\n", s_tests_passed);
    printf("  Failed: %d\n", s_tests_failed);

    if (s_tests_failed == 0) {
        printf("\nALL TESTS PASSED SUCCESSFULLY (Hito 1, 2 & 4 Core verified).\n");
        return 0;
    } else {
        printf("\nTEST SUITE FAILED.\n");
        return 1;
    }
}
