#ifndef TETRIS_DX_HOOKS_H
#define TETRIS_DX_HOOKS_H

#include "gbc_types.h"
#include "gbc_video.h"

void tetris_dx_hooks_init(void);
void tetris_dx_hook_sync_save_ram(const uint8_t *sram_data, uint32_t size);
void tetris_dx_hook_setup_side_borders(void);

#endif
