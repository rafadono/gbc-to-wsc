#ifndef GBC_AUDIO_H
#define GBC_AUDIO_H

#include "gbc_types.h"
#include "wsc_hardware.h"

typedef struct {
    uint8_t nr10;
    uint8_t nr11;
    uint8_t nr12;
    uint8_t nr13;
    uint8_t nr14;
    uint16_t ch1_freq;
    uint8_t ch1_vol;
    uint8_t ch1_env_timer;
    uint8_t ch1_length;
    bool ch1_active;
    bool ch1_length_enable;

    uint8_t nr21;
    uint8_t nr22;
    uint8_t nr23;
    uint8_t nr24;
    uint16_t ch2_freq;
    uint8_t ch2_vol;
    uint8_t ch2_env_timer;
    uint8_t ch2_length;
    bool ch2_active;
    bool ch2_length_enable;

    uint8_t nr30;
    uint8_t nr31;
    uint8_t nr32;
    uint8_t nr33;
    uint8_t nr34;
    uint16_t ch3_freq;
    uint8_t ch3_vol;
    uint16_t ch3_length;
    bool ch3_active;
    bool ch3_length_enable;
    uint8_t wave_ram[16];

    uint8_t nr41;
    uint8_t nr42;
    uint8_t nr43;
    uint8_t nr44;
    uint8_t ch4_vol;
    uint8_t ch4_env_timer;
    uint8_t ch4_length;
    bool ch4_active;
    bool ch4_length_enable;

    uint8_t nr50;
    uint8_t nr51;
    uint8_t nr52;

    uint8_t wsc_ch1_pitch_l;
    uint8_t wsc_ch1_pitch_h;
    uint8_t wsc_ch2_pitch_l;
    uint8_t wsc_ch2_pitch_h;
    uint8_t wsc_ch3_pitch_l;
    uint8_t wsc_ch3_pitch_h;
    uint8_t wsc_ch4_pitch_l;
    uint8_t wsc_ch4_pitch_h;
    uint8_t wsc_vol_ch1_ch2;
    uint8_t wsc_vol_ch3_ch4;
    uint8_t wsc_sweep_step;
    uint8_t wsc_sweep_time;
    uint8_t wsc_noise_ctrl;
    uint8_t wsc_output_mode;
    uint8_t wsc_master_vol;

    uint8_t wsc_ch1_wave[16];
    uint8_t wsc_ch2_wave[16];
} gbc_audio_state_t;

extern gbc_audio_state_t g_audio;

void gbc_audio_init(void);
void gbc_audio_reset(void);

void gbc_audio_write_reg(uint16_t addr, uint8_t val);
uint8_t gbc_audio_read_reg(uint16_t addr);

void gbc_audio_step(uint16_t cycles);
void gbc_audio_sync_wsc(void);

uint16_t gbc_to_wsc_pitch(uint16_t gb_freq, bool is_wave);

void gbc_audio_set_enhanced(bool enabled);
bool gbc_audio_get_enhanced(void);

typedef struct {
    uint8_t buffer[2048];
    uint16_t write_pos;
    uint16_t read_pos;
    uint16_t count;
} gbc_audio_ring_buffer_t;

extern gbc_audio_ring_buffer_t g_audio_ring;

void gbc_audio_ring_init(void);
bool gbc_audio_ring_write(uint8_t sample);
uint8_t gbc_audio_ring_read(void);
uint16_t gbc_audio_ring_available(void);

#endif
