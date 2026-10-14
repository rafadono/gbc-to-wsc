#ifndef SML_HOOKS_H
#define SML_HOOKS_H

#include <stdint.h>
#include <stdbool.h>

#define SML_VANILLA_VIEWPORT_TILES 20
#define SML_WIDE_VIEWPORT_TILES    28

#define SML_VANILLA_SCROLL_TRIGGER 0x50  /* 80 px - center of 160 px screen */
#define SML_WIDE_SCROLL_TRIGGER    0x70  /* 112 px - center of 224 px screen */

#define SML_VANILLA_INIT_COLUMNS   0x54  /* 0x40 + 20 columns */
#define SML_WIDE_INIT_COLUMNS      0x5C  /* 0x40 + 28 columns */

void sml_hooks_init(void);

/* Returns 0x5C (28 cols) in widescreen or default 0x54 (20 cols) */
uint8_t sml_hook_initial_column_end(uint8_t default_val);

/* Returns 0x70 (112 px) in widescreen or default 0x50 (80 px) */
uint8_t sml_hook_scroll_threshold(uint8_t default_val);

/* Returns camera width in tiles (28 wide vs 20 standard) */
uint8_t sml_hook_get_camera_width_tiles(void);

/* Enemy spawn and activation boundary check */
bool sml_hook_is_enemy_in_active_window(int16_t enemy_x, int16_t camera_x);

/* Centers top HUD row across 28 tiles in widescreen mode */
void sml_hook_update_hud_layout(uint8_t *hud_tilemap_row);

#endif
