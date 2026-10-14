#include <string.h>
#include "gbc_audio.h"
#include "gbc_memory.h"

gbc_audio_state_t g_audio;

uint16_t gbc_to_wsc_pitch(uint16_t gb_freq, bool is_wave) {
    if (gb_freq >= 2048) {
        gb_freq = 2047;
    }
    uint32_t diff = 2048 - gb_freq;
    uint32_t period;
    if (!is_wave) {
        period = (diff * 375 + 256) / 512;
    } else {
        period = (diff * 375 + 128) / 256;
    }
    if (period < 1) period = 1;
    if (period > 2047) period = 2047;
    return (uint16_t)(2048 - period);
}

void gbc_audio_init(void) {
    memset(&g_audio, 0, sizeof(gbc_audio_state_t));
    g_audio.nr52 = 0x80;
    g_audio.nr50 = 0x77;
    g_audio.nr51 = 0xF3;
    gbc_audio_sync_wsc();
}

void gbc_audio_reset(void) {
    gbc_audio_init();
}

void gbc_audio_sync_wsc(void) {
    uint16_t p1 = gbc_to_wsc_pitch(g_audio.ch1_freq, false);
    g_audio.wsc_ch1_pitch_l = (uint8_t)(p1 & 0xFF);
    g_audio.wsc_ch1_pitch_h = (uint8_t)((p1 >> 8) & 0x07);

    uint16_t p2 = gbc_to_wsc_pitch(g_audio.ch2_freq, false);
    g_audio.wsc_ch2_pitch_l = (uint8_t)(p2 & 0xFF);
    g_audio.wsc_ch2_pitch_h = (uint8_t)((p2 >> 8) & 0x07);

    uint16_t p3 = gbc_to_wsc_pitch(g_audio.ch3_freq, true);
    g_audio.wsc_ch3_pitch_l = (uint8_t)(p3 & 0xFF);
    g_audio.wsc_ch3_pitch_h = (uint8_t)((p3 >> 8) & 0x07);

    g_audio.wsc_ch4_pitch_l = g_audio.nr43;
    g_audio.wsc_ch4_pitch_h = 0;

    uint8_t v1 = g_audio.ch1_active ? g_audio.ch1_vol : 0;
    uint8_t v2 = g_audio.ch2_active ? g_audio.ch2_vol : 0;
    uint8_t v3 = g_audio.ch3_active ? g_audio.ch3_vol : 0;
    uint8_t v4 = g_audio.ch4_active ? g_audio.ch4_vol : 0;

    g_audio.wsc_vol_ch1_ch2 = (v2 << 4) | (v1 & 0x0F);
    g_audio.wsc_vol_ch3_ch4 = (v4 << 4) | (v3 & 0x0F);

    uint8_t sweep_shift = g_audio.nr10 & 0x07;
    bool sweep_neg = (g_audio.nr10 & 0x08) != 0;
    g_audio.wsc_sweep_step = sweep_neg ? (uint8_t)(-(int8_t)sweep_shift) : sweep_shift;
    g_audio.wsc_sweep_time = (g_audio.nr10 >> 4) & 0x07;

    uint8_t noise_ctrl = 0;
    if (g_audio.ch4_active) noise_ctrl |= 0x01;
    if (g_audio.nr43 & 0x08) noise_ctrl |= 0x02;
    g_audio.wsc_noise_ctrl = noise_ctrl;

    g_audio.wsc_output_mode = g_audio.nr51;

    uint8_t left_v = (g_audio.nr50 >> 4) & 0x07;
    uint8_t right_v = g_audio.nr50 & 0x07;
    g_audio.wsc_master_vol = (left_v > right_v ? left_v : right_v) >> 1;
}

void gbc_audio_write_reg(uint16_t addr, uint8_t val) {
    if (addr >= GB_REG_WAVE_START && addr <= GB_REG_WAVE_END) {
        g_audio.wave_ram[addr - GB_REG_WAVE_START] = val;
        return;
    }

    if (!(g_audio.nr52 & 0x80) && addr != GB_REG_NR52) {
        return;
    }

    switch (addr) {
        case GB_REG_NR10:
            g_audio.nr10 = val;
            break;
        case GB_REG_NR11:
            g_audio.nr11 = val;
            g_audio.ch1_length = 64 - (val & 0x3F);
            break;
        case GB_REG_NR12:
            g_audio.nr12 = val;
            g_audio.ch1_vol = val >> 4;
            break;
        case GB_REG_NR13:
            g_audio.nr13 = val;
            g_audio.ch1_freq = (g_audio.ch1_freq & 0x0700) | val;
            break;
        case GB_REG_NR14:
            g_audio.nr14 = val;
            g_audio.ch1_freq = (g_audio.ch1_freq & 0x00FF) | ((uint16_t)(val & 0x07) << 8);
            g_audio.ch1_length_enable = (val & 0x40) != 0;
            if (val & 0x80) {
                g_audio.ch1_active = true;
                if (g_audio.ch1_length == 0) g_audio.ch1_length = 64;
                g_audio.ch1_vol = g_audio.nr12 >> 4;
            }
            break;

        case GB_REG_NR21:
            g_audio.nr21 = val;
            g_audio.ch2_length = 64 - (val & 0x3F);
            break;
        case GB_REG_NR22:
            g_audio.nr22 = val;
            g_audio.ch2_vol = val >> 4;
            break;
        case GB_REG_NR23:
            g_audio.nr23 = val;
            g_audio.ch2_freq = (g_audio.ch2_freq & 0x0700) | val;
            break;
        case GB_REG_NR24:
            g_audio.nr24 = val;
            g_audio.ch2_freq = (g_audio.ch2_freq & 0x00FF) | ((uint16_t)(val & 0x07) << 8);
            g_audio.ch2_length_enable = (val & 0x40) != 0;
            if (val & 0x80) {
                g_audio.ch2_active = true;
                if (g_audio.ch2_length == 0) g_audio.ch2_length = 64;
                g_audio.ch2_vol = g_audio.nr22 >> 4;
            }
            break;

        case GB_REG_NR30:
            g_audio.nr30 = val;
            if (!(val & 0x80)) g_audio.ch3_active = false;
            break;
        case GB_REG_NR31:
            g_audio.nr31 = val;
            g_audio.ch3_length = 256 - val;
            break;
        case GB_REG_NR32: {
            g_audio.nr32 = val;
            uint8_t shift = (val >> 5) & 0x03;
            if (shift == 0) g_audio.ch3_vol = 0;
            else if (shift == 1) g_audio.ch3_vol = 15;
            else if (shift == 2) g_audio.ch3_vol = 8;
            else g_audio.ch3_vol = 4;
            break;
        }
        case GB_REG_NR33:
            g_audio.nr33 = val;
            g_audio.ch3_freq = (g_audio.ch3_freq & 0x0700) | val;
            break;
        case GB_REG_NR34:
            g_audio.nr34 = val;
            g_audio.ch3_freq = (g_audio.ch3_freq & 0x00FF) | ((uint16_t)(val & 0x07) << 8);
            g_audio.ch3_length_enable = (val & 0x40) != 0;
            if (val & 0x80) {
                g_audio.ch3_active = (g_audio.nr30 & 0x80) != 0;
                if (g_audio.ch3_length == 0) g_audio.ch3_length = 256;
            }
            break;

        case GB_REG_NR41:
            g_audio.nr41 = val;
            g_audio.ch4_length = 64 - (val & 0x3F);
            break;
        case GB_REG_NR42:
            g_audio.nr42 = val;
            g_audio.ch4_vol = val >> 4;
            break;
        case GB_REG_NR43:
            g_audio.nr43 = val;
            break;
        case GB_REG_NR44:
            g_audio.nr44 = val;
            g_audio.ch4_length_enable = (val & 0x40) != 0;
            if (val & 0x80) {
                g_audio.ch4_active = true;
                if (g_audio.ch4_length == 0) g_audio.ch4_length = 64;
                g_audio.ch4_vol = g_audio.nr42 >> 4;
            }
            break;

        case GB_REG_NR50:
            g_audio.nr50 = val;
            break;
        case GB_REG_NR51:
            g_audio.nr51 = val;
            break;
        case GB_REG_NR52:
            g_audio.nr52 = (g_audio.nr52 & 0x7F) | (val & 0x80);
            if (!(val & 0x80)) {
                g_audio.ch1_active = false;
                g_audio.ch2_active = false;
                g_audio.ch3_active = false;
                g_audio.ch4_active = false;
                g_audio.nr10 = 0; g_audio.nr11 = 0; g_audio.nr12 = 0; g_audio.nr13 = 0; g_audio.nr14 = 0;
                g_audio.nr21 = 0; g_audio.nr22 = 0; g_audio.nr23 = 0; g_audio.nr24 = 0;
                g_audio.nr30 = 0; g_audio.nr31 = 0; g_audio.nr32 = 0; g_audio.nr33 = 0; g_audio.nr34 = 0;
                g_audio.nr41 = 0; g_audio.nr42 = 0; g_audio.nr43 = 0; g_audio.nr44 = 0;
                g_audio.nr50 = 0; g_audio.nr51 = 0;
            }
            break;
    }

    gbc_audio_sync_wsc();
}

uint8_t gbc_audio_read_reg(uint16_t addr) {
    if (addr >= GB_REG_WAVE_START && addr <= GB_REG_WAVE_END) {
        return g_audio.wave_ram[addr - GB_REG_WAVE_START];
    }

    switch (addr) {
        case GB_REG_NR10: return g_audio.nr10 | 0x80;
        case GB_REG_NR11: return g_audio.nr11 | 0x3F;
        case GB_REG_NR12: return g_audio.nr12;
        case GB_REG_NR13: return 0xFF;
        case GB_REG_NR14: return g_audio.nr14 | 0xBF;

        case GB_REG_NR21: return g_audio.nr21 | 0x3F;
        case GB_REG_NR22: return g_audio.nr22;
        case GB_REG_NR23: return 0xFF;
        case GB_REG_NR24: return g_audio.nr24 | 0xBF;

        case GB_REG_NR30: return g_audio.nr30 | 0x7F;
        case GB_REG_NR31: return 0xFF;
        case GB_REG_NR32: return g_audio.nr32 | 0x9F;
        case GB_REG_NR33: return 0xFF;
        case GB_REG_NR34: return g_audio.nr34 | 0xBF;

        case GB_REG_NR41: return 0xFF;
        case GB_REG_NR42: return g_audio.nr42;
        case GB_REG_NR43: return g_audio.nr43;
        case GB_REG_NR44: return g_audio.nr44 | 0xBF;

        case GB_REG_NR50: return g_audio.nr50;
        case GB_REG_NR51: return g_audio.nr51;
        case GB_REG_NR52: {
            uint8_t status = (g_audio.nr52 & 0x80) | 0x70;
            if (g_audio.ch1_active) status |= 0x01;
            if (g_audio.ch2_active) status |= 0x02;
            if (g_audio.ch3_active) status |= 0x04;
            if (g_audio.ch4_active) status |= 0x08;
            return status;
        }
        default:
            return 0xFF;
    }
}

void gbc_audio_step(uint16_t cycles) {
    (void)cycles;
    gbc_audio_sync_wsc();
}
