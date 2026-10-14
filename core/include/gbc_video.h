#ifndef GBC_VIDEO_H
#define GBC_VIDEO_H

#include "gbc_types.h"
#include "wsc_hardware.h"

typedef enum {
    GBC_SCREEN_MODE_STANDARD = 0,   /* 160x144 centered with 32px black borders */
    GBC_SCREEN_MODE_WIDESCREEN = 1  /* 224x144 full native viewport */
} gbc_screen_mode_t;

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

    uint8_t bcps;
    uint8_t ocps;
    uint8_t cgb_bg_pal[64];
    uint8_t cgb_obj_pal[64];

    uint16_t hdma_src;
    uint16_t hdma_dst;
    uint8_t hdma5;
    bool hdma_active;
    uint8_t hdma_blocks_remaining;

    bool vram_dirty;
    uint8_t tile_dirty[384];

    uint16_t wsc_bg_palettes[16][16];
    uint16_t wsc_obj_palettes[16][16];

    wsc_sprite_t wsc_sprites[128];
    uint8_t wsc_sprite_count;

    bool window_enabled;
    uint8_t win_x;
    uint8_t win_y;

    uint16_t wsc_disp_ctrl;
    uint8_t wsc_win_x1;
    uint8_t wsc_win_y1;
    uint8_t wsc_win_x2;
    uint8_t wsc_win_y2;

    gbc_screen_mode_t screen_mode;
    uint8_t viewport_x_offset;
    uint8_t viewport_width;
} gbc_video_state_t;

extern gbc_video_state_t g_video;

void gbc_video_init(void);
void gbc_video_reset(void);

void gbc_video_set_screen_mode(gbc_screen_mode_t mode);
gbc_screen_mode_t gbc_video_get_screen_mode(void);
uint8_t gbc_video_get_viewport_x_offset(void);
uint8_t gbc_video_get_viewport_width(void);

void gbc_tile_to_wsc_2bpp(const uint8_t *gb_tile, uint8_t *wsc_tile);
void gbc_video_mark_tile_dirty(uint16_t tile_idx);

void gbc_video_update_palettes(void);
void gbc_video_update_sprites(void);
void gbc_video_update_window(void);
void gbc_video_update_window_screen2(void);
void gbc_video_render_frame(void);

void gbc_video_step_scanline(void);

void gbc_video_write_bcps(uint8_t val);
void gbc_video_write_bcpd(uint8_t val);
uint8_t gbc_video_read_bcps(void);
uint8_t gbc_video_read_bcpd(void);

void gbc_video_write_ocps(uint8_t val);
void gbc_video_write_ocpd(uint8_t val);
uint8_t gbc_video_read_ocps(void);
uint8_t gbc_video_read_ocpd(void);

void gbc_video_write_hdma1(uint8_t val);
void gbc_video_write_hdma2(uint8_t val);
void gbc_video_write_hdma3(uint8_t val);
void gbc_video_write_hdma4(uint8_t val);
void gbc_video_write_hdma5(uint8_t val);
uint8_t gbc_video_read_hdma5(void);
void gbc_video_step_hdma_hblank(void);

void gbc_video_cycle_dmg_palette(void);
void gbc_video_set_dmg_palette_preset(dmg_palette_preset_t preset);
dmg_palette_preset_t gbc_video_get_dmg_palette_preset(void);
const char *gbc_video_get_dmg_palette_name(dmg_palette_preset_t preset);

void gbc_video_set_orientation(gbc_orientation_t orientation);
gbc_orientation_t gbc_video_get_orientation(void);

#endif
