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
    g_video.vram_dirty = true;
    memset(g_video.tile_dirty, 1, sizeof(g_video.tile_dirty));

    gbc_video_update_palettes();
}

void gbc_video_reset(void) {
    gbc_video_init();
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

    if (g_video.ly == 144) {
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

        int wsc_x = (int)entry->x - 8 + GB_VIEWPORT_X_OFFSET;
        int wsc_y = (int)entry->y - 16;

        if (wsc_x < -7 || wsc_x >= WSC_SCREEN_W || wsc_y < -15 || wsc_y >= WSC_SCREEN_H) {
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
    g_video.window_enabled = (g_video.lcdc & GB_LCDC_WIN_ENABLE) != 0;
    if (g_video.window_enabled) {
        g_video.win_y = g_video.wy;
        g_video.win_x = (g_video.wx >= 7) ? (g_video.wx - 7 + GB_VIEWPORT_X_OFFSET) : GB_VIEWPORT_X_OFFSET;
    }
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
    gbc_video_update_window();
}
