#include <string.h>
#include "gbc_video.h"
#include "gbc_memory.h"
#include "gbc_hal.h"

gbc_video_state_t g_video;

static const uint16_t DMG_PALETTE_COLORS[4] = {
    0x0FFF,
    0x0AAA,
    0x0555,
    0x0000
};

void gbc_video_init(void) {
    memset(&g_video, 0, sizeof(gbc_video_state_t));
    g_video.lcdc = 0x91;
    g_video.bgp  = 0xFC;
    g_video.obp0 = 0xFF;
    g_video.obp1 = 0xFF;
    g_video.hdma5 = 0xFF;
    g_video.wsc_disp_ctrl = WSC_DISP_CTRL_SCR1_ON | WSC_DISP_CTRL_SPR_ON;
    g_video.vram_dirty = true;
    memset(g_video.tile_dirty, 1, sizeof(g_video.tile_dirty));

    g_video.screen_mode = GBC_SCREEN_MODE_STANDARD;
    g_video.viewport_x_offset = GB_VIEWPORT_X_OFFSET;
    g_video.viewport_width = GB_SCREEN_W;

    gbc_video_update_palettes();
}

void gbc_video_reset(void) {
    gbc_screen_mode_t prev_mode = g_video.screen_mode;
    gbc_video_init();
    gbc_video_set_screen_mode(prev_mode);
}

void gbc_video_set_screen_mode(gbc_screen_mode_t mode) {
    g_video.screen_mode = mode;
    if (mode == GBC_SCREEN_MODE_WIDESCREEN) {
        g_video.viewport_x_offset = 0;
        g_video.viewport_width = WSC_SCREEN_W;
    } else {
        g_video.viewport_x_offset = GB_VIEWPORT_X_OFFSET;
        g_video.viewport_width = GB_SCREEN_W;
    }
}

gbc_screen_mode_t gbc_video_get_screen_mode(void) {
    return g_video.screen_mode;
}

uint8_t gbc_video_get_viewport_x_offset(void) {
    return g_video.viewport_x_offset;
}

uint8_t gbc_video_get_viewport_width(void) {
    return g_video.viewport_width;
}

void gbc_tile_to_wsc_2bpp(const uint8_t *gb_tile, uint8_t *wsc_tile) {
    for (int y = 0; y < 8; y++) {
        wsc_tile[y] = gb_tile[y * 2];
        wsc_tile[y + 8] = gb_tile[y * 2 + 1];
    }
}

void gbc_video_mark_tile_dirty(uint16_t tile_idx) {
    if (tile_idx < 384) {
        g_video.tile_dirty[tile_idx] = 1;
        g_video.vram_dirty = true;
    }
}

void gbc_video_update_palettes(void) {
    for (int i = 0; i < 4; i++) {
        uint8_t shade = (g_video.bgp >> (i * 2)) & 0x03;
        g_video.wsc_bg_palettes[0][i] = DMG_PALETTE_COLORS[shade];
    }

    for (int i = 0; i < 4; i++) {
        uint8_t shade0 = (g_video.obp0 >> (i * 2)) & 0x03;
        uint8_t shade1 = (g_video.obp1 >> (i * 2)) & 0x03;
        g_video.wsc_obj_palettes[0][i] = DMG_PALETTE_COLORS[shade0];
        g_video.wsc_obj_palettes[1][i] = DMG_PALETTE_COLORS[shade1];
    }
}

void gbc_video_step_scanline(void) {
    g_video.ly++;

    if (g_video.ly < 144) {
        gbc_video_step_hdma_hblank();
    } else if (g_video.ly == 144) {
        g_video.stat = (g_video.stat & ~GB_STAT_MODE_MASK) | 0x01;
        gbc_hal_request_interrupt(GB_INT_VBLANK);
        if (g_video.stat & GB_STAT_VBLANK_INT) {
            gbc_hal_request_interrupt(GB_INT_LCD_STAT);
        }
    } else if (g_video.ly > 153) {
        g_video.ly = 0;
        g_video.stat = (g_video.stat & ~GB_STAT_MODE_MASK) | 0x02;
    }

    if (g_video.ly == g_video.lyc) {
        g_video.stat |= GB_STAT_LYC_FLAG;
        if (g_video.stat & GB_STAT_LYC_INT) {
            gbc_hal_request_interrupt(GB_INT_LCD_STAT);
        }
    } else {
        g_video.stat &= ~GB_STAT_LYC_FLAG;
    }
}

void gbc_video_update_sprites(void) {
    if (!(g_video.lcdc & GB_LCDC_OBJ_ENABLE)) {
        g_video.wsc_sprite_count = 0;
        return;
    }

    bool is_8x16 = (g_video.lcdc & GB_LCDC_OBJ_SIZE) != 0;
    uint8_t count = 0;

    for (int i = 0; i < 40 && count < 126; i++) {
        const gb_oam_entry_t *entry = (const gb_oam_entry_t *)&g_gb.oam[i * 4];
        if (entry->y == 0 || entry->y >= 160) {
            continue;
        }

        int wsc_x = (int)entry->x - 8 + g_video.viewport_x_offset;
        int wsc_y = (int)entry->y - 16;

        int min_x = (g_video.screen_mode == GBC_SCREEN_MODE_STANDARD) ? (GB_VIEWPORT_X_OFFSET - 7) : -7;
        int max_x = (g_video.screen_mode == GBC_SCREEN_MODE_STANDARD) ? (GB_VIEWPORT_X_OFFSET + GB_SCREEN_W) : WSC_SCREEN_W;

        if (wsc_x < min_x || wsc_x >= max_x || wsc_y < -15 || wsc_y >= WSC_SCREEN_H) {
            continue;
        }

        uint8_t pal = (entry->flags & 0x10) ? 9 : 8;
        uint8_t flags = pal & WSC_SPR_ATTR_PAL_MASK;

        if (entry->flags & 0x20) flags |= WSC_SPR_ATTR_HFLIP;
        if (entry->flags & 0x40) flags |= WSC_SPR_ATTR_VFLIP;
        if (!(entry->flags & 0x80)) flags |= (WSC_SPR_ATTR_PRIO_SCR1 | WSC_SPR_ATTR_PRIO_SCR2);

        if (!is_8x16) {
            g_video.wsc_sprites[count].tile = entry->tile;
            g_video.wsc_sprites[count].flags = flags;
            g_video.wsc_sprites[count].x = (uint8_t)wsc_x;
            g_video.wsc_sprites[count].y = (uint8_t)wsc_y;
            count++;
        } else {
            uint8_t base_tile = entry->tile & 0xFE;
            bool vflip = (entry->flags & 0x40) != 0;

            g_video.wsc_sprites[count].tile = vflip ? (base_tile | 1) : base_tile;
            g_video.wsc_sprites[count].flags = flags;
            g_video.wsc_sprites[count].x = (uint8_t)wsc_x;
            g_video.wsc_sprites[count].y = (uint8_t)wsc_y;
            count++;

            g_video.wsc_sprites[count].tile = vflip ? base_tile : (base_tile | 1);
            g_video.wsc_sprites[count].flags = flags;
            g_video.wsc_sprites[count].x = (uint8_t)wsc_x;
            g_video.wsc_sprites[count].y = (uint8_t)(wsc_y + 8);
            count++;
        }
    }

    g_video.wsc_sprite_count = count;
}

void gbc_video_update_window(void) {
    gbc_video_update_window_screen2();
}

void gbc_video_update_window_screen2(void) {
    g_video.window_enabled = ((g_video.lcdc & GB_LCDC_WIN_ENABLE) != 0) &&
                            ((g_video.lcdc & GB_LCDC_ENABLE) != 0) &&
                            (g_video.wy < GB_SCREEN_H) &&
                            (g_video.wx < 167);

    if (!g_video.window_enabled) {
        g_video.wsc_disp_ctrl &= ~(WSC_DISP_CTRL_SCR2_ON | WSC_DISP_CTRL_WIN_ON);
        g_video.wsc_win_x1 = 0;
        g_video.wsc_win_y1 = 0;
        g_video.wsc_win_x2 = 0;
        g_video.wsc_win_y2 = 0;
        return;
    }

    g_video.wsc_disp_ctrl |= (WSC_DISP_CTRL_SCR2_ON | WSC_DISP_CTRL_WIN_ON);

    int left = (g_video.wx >= 7) ? (int)(g_video.wx - 7) : 0;
    int wsc_left = left + g_video.viewport_x_offset;
    int wsc_right = g_video.viewport_x_offset + g_video.viewport_width - 1;

    if (wsc_left > wsc_right) {
        wsc_left = wsc_right;
    }

    g_video.win_x = (uint8_t)wsc_left;
    g_video.win_y = g_video.wy;

    g_video.wsc_win_x1 = (uint8_t)wsc_left;
    g_video.wsc_win_y1 = g_video.wy;
    g_video.wsc_win_x2 = (uint8_t)wsc_right;
    g_video.wsc_win_y2 = (uint8_t)(WSC_SCREEN_H - 1);
}

void gbc_video_render_frame(void) {
    if (!(g_video.lcdc & GB_LCDC_ENABLE)) {
        return;
    }

    if (g_video.vram_dirty) {
        for (int i = 0; i < 384; i++) {
            if (g_video.tile_dirty[i]) {
                g_video.tile_dirty[i] = 0;
            }
        }
        g_video.vram_dirty = false;
    }

    gbc_video_update_sprites();
    gbc_video_update_window_screen2();
}

void gbc_video_write_bcps(uint8_t val) {
    g_video.bcps = val;
}

void gbc_video_write_bcpd(uint8_t val) {
    uint8_t idx = g_video.bcps & 0x3F;
    g_video.cgb_bg_pal[idx] = val;

    uint8_t pal_num = idx >> 3;
    uint8_t col_num = (idx >> 1) & 0x03;
    uint8_t base = (pal_num << 3) | (col_num << 1);
    uint16_t bgr555 = (uint16_t)g_video.cgb_bg_pal[base] | ((uint16_t)g_video.cgb_bg_pal[base + 1] << 8);
    g_video.wsc_bg_palettes[pal_num][col_num] = gbc_to_wsc_color(bgr555);

    if (g_video.bcps & 0x80) {
        g_video.bcps = (g_video.bcps & 0x80) | ((idx + 1) & 0x3F);
    }
}

uint8_t gbc_video_read_bcps(void) {
    return g_video.bcps | 0x40;
}

uint8_t gbc_video_read_bcpd(void) {
    return g_video.cgb_bg_pal[g_video.bcps & 0x3F];
}

void gbc_video_write_ocps(uint8_t val) {
    g_video.ocps = val;
}

void gbc_video_write_ocpd(uint8_t val) {
    uint8_t idx = g_video.ocps & 0x3F;
    g_video.cgb_obj_pal[idx] = val;

    uint8_t pal_num = idx >> 3;
    uint8_t col_num = (idx >> 1) & 0x03;
    uint8_t base = (pal_num << 3) | (col_num << 1);
    uint16_t bgr555 = (uint16_t)g_video.cgb_obj_pal[base] | ((uint16_t)g_video.cgb_obj_pal[base + 1] << 8);
    g_video.wsc_obj_palettes[pal_num][col_num] = gbc_to_wsc_color(bgr555);

    if (g_video.ocps & 0x80) {
        g_video.ocps = (g_video.ocps & 0x80) | ((idx + 1) & 0x3F);
    }
}

uint8_t gbc_video_read_ocps(void) {
    return g_video.ocps | 0x40;
}

uint8_t gbc_video_read_ocpd(void) {
    return g_video.cgb_obj_pal[g_video.ocps & 0x3F];
}

void gbc_video_write_hdma1(uint8_t val) {
    g_video.hdma_src = (g_video.hdma_src & 0x00F0) | ((uint16_t)val << 8);
}

void gbc_video_write_hdma2(uint8_t val) {
    g_video.hdma_src = (g_video.hdma_src & 0xFF00) | (val & 0xF0);
}

void gbc_video_write_hdma3(uint8_t val) {
    g_video.hdma_dst = 0x8000 | ((g_video.hdma_dst & 0x00F0) | ((uint16_t)(val & 0x1F) << 8));
}

void gbc_video_write_hdma4(uint8_t val) {
    g_video.hdma_dst = (g_video.hdma_dst & 0xFF00) | (val & 0xF0);
}

static void gbc_video_transfer_hdma_block(void) {
    uint16_t src = g_video.hdma_src;
    uint16_t dst = g_video.hdma_dst;

    for (int i = 0; i < 16; i++) {
        uint8_t byte = gbc_read8(src + i);
        uint16_t vram_offset = (dst + i) - 0x8000;
        if (g_gb.current_vram_bank == 1 && g_gb.is_cgb) {
            vram_offset += 0x2000;
        }
        if (vram_offset < sizeof(g_gb.vram)) {
            g_gb.vram[vram_offset] = byte;
            if (vram_offset < 0x1800) {
                gbc_video_mark_tile_dirty(vram_offset >> 4);
            }
        }
    }

    g_video.hdma_src += 16;
    g_video.hdma_dst += 16;
    if (g_video.hdma_dst >= 0xA000) {
        g_video.hdma_dst = 0x8000 + (g_video.hdma_dst & 0x1FFF);
    }
}

void gbc_video_write_hdma5(uint8_t val) {
    if (g_video.hdma_active && !(val & 0x80)) {
        g_video.hdma_active = false;
        g_video.hdma5 = 0x80 | g_video.hdma_blocks_remaining;
        return;
    }

    uint8_t blocks = (val & 0x7F) + 1;
    bool is_hblank = (val & 0x80) != 0;

    if (!is_hblank) {
        for (uint8_t b = 0; b < blocks; b++) {
            gbc_video_transfer_hdma_block();
        }
        g_video.hdma_active = false;
        g_video.hdma_blocks_remaining = 0;
        g_video.hdma5 = 0xFF;
    } else {
        g_video.hdma_active = true;
        g_video.hdma_blocks_remaining = blocks;
        g_video.hdma5 = val & 0x7F;
    }
}

uint8_t gbc_video_read_hdma5(void) {
    if (!g_video.hdma_active) {
        return 0xFF;
    }
    return (g_video.hdma_blocks_remaining > 0 ? (g_video.hdma_blocks_remaining - 1) : 0);
}

void gbc_video_step_hdma_hblank(void) {
    if (g_video.hdma_active && g_video.hdma_blocks_remaining > 0) {
        gbc_video_transfer_hdma_block();
        g_video.hdma_blocks_remaining--;
        if (g_video.hdma_blocks_remaining == 0) {
            g_video.hdma_active = false;
            g_video.hdma5 = 0xFF;
        } else {
            g_video.hdma5 = g_video.hdma_blocks_remaining - 1;
        }
    }
}
