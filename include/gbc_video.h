#ifndef GBC_VIDEO_H
#define GBC_VIDEO_H

#include "gbc_types.h"
#include "wsc_hardware.h"

typedef struct {
    uint8_t scx;
    uint8_t scy;
    uint8_t wx;
    uint8_t wy;
    uint8_t ly;
    uint8_t lyc;
    uint8_t lcdc;
    uint8_t stat;
    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;

    uint8_t cgb_bg_pal[64];
    uint8_t cgb_obj_pal[64];

    bool vram_dirty;
    uint8_t tile_dirty[384];

    uint16_t wsc_bg_palettes[16][16];
    uint16_t wsc_obj_palettes[16][16];

    wsc_sprite_t wsc_sprites[128];
    uint8_t wsc_sprite_count;

    bool window_enabled;
    uint8_t win_x;
    uint8_t win_y;
} gbc_video_state_t;

extern gbc_video_state_t g_video;

void gbc_video_init(void);
void gbc_video_reset(void);

void gbc_tile_to_wsc_2bpp(const uint8_t *gb_tile, uint8_t *wsc_tile);
void gbc_video_mark_tile_dirty(uint16_t tile_idx);

void gbc_video_update_palettes(void);
void gbc_video_update_sprites(void);
void gbc_video_update_window(void);
void gbc_video_render_frame(void);

void gbc_video_step_scanline(void);

#endif
