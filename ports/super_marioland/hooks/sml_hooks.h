#ifndef SML_HOOKS_H
#define SML_HOOKS_H

#include "gbc_types.h"
#include "gbc_video.h"

#define SML_VANILLA_VIEWPORT_TILES 20
#define SML_WIDE_VIEWPORT_TILES    28

void sml_hooks_init(void);
uint8_t sml_hook_get_camera_width_tiles(void);
bool sml_hook_is_enemy_in_active_window(int16_t enemy_x, int16_t camera_x);
void sml_hook_update_hud_layout(uint8_t *hud_tilemap_row);

#endif
