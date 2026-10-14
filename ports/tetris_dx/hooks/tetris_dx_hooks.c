#include "tetris_dx_hooks.h"
#include <string.h>

void tetris_dx_hooks_init(void) {
    tetris_dx_hook_setup_side_borders();
}

void tetris_dx_hook_sync_save_ram(const uint8_t *sram_data, uint32_t size) {
    (void)sram_data;
    (void)size;
}

void tetris_dx_hook_setup_side_borders(void) {
    gbc_video_set_screen_mode(GBC_SCREEN_MODE_STANDARD);
}
