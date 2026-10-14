#include "sml_hooks.h"
#include "gbc_video.h"
#include <string.h>

void sml_hooks_init(void) {
}

uint8_t sml_hook_initial_column_end(uint8_t default_val) {
    if (gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN) {
        return SML_WIDE_INIT_COLUMNS;
    }
    return default_val;
}

uint8_t sml_hook_scroll_threshold(uint8_t default_val) {
    if (gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN) {
        return SML_WIDE_SCROLL_TRIGGER;
    }
    return default_val;
}

uint8_t sml_hook_get_camera_width_tiles(void) {
    if (gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN) {
        return SML_WIDE_VIEWPORT_TILES;
    }
    return SML_VANILLA_VIEWPORT_TILES;
}

bool sml_hook_is_enemy_in_active_window(int16_t enemy_x, int16_t camera_x) {
    int16_t rel_x = enemy_x - camera_x;
    int16_t max_visible = (gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN) ? 224 : 160;
    return (rel_x >= -16 && rel_x <= (max_visible + 16));
}

void sml_hook_update_hud_layout(uint8_t *hud_tilemap_row) {
    if (gbc_video_get_screen_mode() == GBC_SCREEN_MODE_WIDESCREEN) {
        uint8_t temp[32];
        memcpy(temp, hud_tilemap_row, 32);
        memset(hud_tilemap_row, 0x00, 32);
        memcpy(&hud_tilemap_row[4], &temp[0], 20);
    }
}
