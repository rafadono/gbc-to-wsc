#include "gbc_hal.h"
#include <string.h>

gbc_hal_context_t g_hal;

void gbc_hal_init(const uint8_t *rom, uint32_t rom_size, uint8_t *sram, uint32_t sram_size) {
    memset(&g_hal, 0, sizeof(gbc_hal_context_t));
    g_hal.is_running = true;

    gbc_memory_init(&g_gb, rom, rom_size, sram, sram_size);
    gbc_video_init();
    gbc_input_init();
}

void gbc_hal_reset(void) {
    gbc_memory_reset(&g_gb);
    gbc_video_reset();
    gbc_input_init();
    g_hal.frame_count = 0;
    g_hal.cycles_executed = 0;
    g_hal.frame_skip_counter = 0;
}

void gbc_hal_request_interrupt(uint8_t int_flag) {
    g_gb.if_reg |= int_flag;
}

void gbc_hal_step_timers(uint16_t cycles) {
    g_gb.div_counter += cycles;

    if (g_gb.tac & 0x04) {
        uint16_t threshold;
        switch (g_gb.tac & 0x03) {
            case 0x00: threshold = 1024; break;
            case 0x01: threshold = 16;   break;
            case 0x02: threshold = 64;   break;
            case 0x03: default: threshold = 256;  break;
        }

        static uint16_t timer_acc = 0;
        timer_acc += cycles;

        while (timer_acc >= threshold) {
            timer_acc -= threshold;
            if (g_gb.tima == 0xFF) {
                g_gb.tima = g_gb.tma;
                gbc_hal_request_interrupt(GB_INT_TIMER);
            } else {
                g_gb.tima++;
            }
        }
    }
}

bool gbc_hal_frame_sync(void) {
    g_hal.frame_skip_counter++;
    if (g_hal.frame_skip_counter >= 5) {
        g_hal.frame_skip_counter = 0;
        return false;
    }
    return true;
}

void gbc_hal_vsync(void) {
    for (int line = 0; line < 154; line++) {
        gbc_video_step_scanline();
        gbc_hal_step_timers(456);
    }
    gbc_video_render_frame();
    g_hal.frame_count++;
}
