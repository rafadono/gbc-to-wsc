#ifndef GBC_HAL_H
#define GBC_HAL_H

#include "gbc_types.h"
#include "gbc_memory.h"
#include "gbc_video.h"
#include "gbc_input.h"
#include "gbc_audio.h"

typedef struct {
    uint32_t frame_count;
    uint32_t cycles_executed;
    uint8_t frame_skip_counter;
    bool is_running;
} gbc_hal_context_t;

extern gbc_hal_context_t g_hal;

void gbc_hal_init(const uint8_t *rom, uint32_t rom_size, uint8_t *sram, uint32_t sram_size);
void gbc_hal_reset(void);

void gbc_hal_request_interrupt(uint8_t int_flag);
void gbc_hal_step_timers(uint16_t cycles);

bool gbc_hal_frame_sync(void);
void gbc_hal_vsync(void);

void gbc_features_init_default(void);
void gbc_features_set_widescreen_toggle(bool enabled);
void gbc_features_set_dmg_palette_cycle(bool enabled);
void gbc_features_set_tate_mode(bool enabled);
void gbc_features_set_enhanced_audio(bool enabled);

#endif
