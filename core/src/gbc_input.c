#include "gbc_input.h"
#include "gbc_memory.h"
#include "gbc_hal.h"
#include "gbc_video.h"

static uint8_t s_buttons_held = 0;
static uint16_t s_prev_wsc_keys = 0;

void gbc_input_init(void) {
    s_buttons_held = 0;
    s_prev_wsc_keys = 0;
}

void gbc_input_set_button(uint8_t button, bool pressed) {
    uint8_t old_buttons = s_buttons_held;
    if (pressed) {
        s_buttons_held |= button;
        if (!(old_buttons & button)) {
            gbc_hal_request_interrupt(GB_INT_JOYPAD);
        }
    } else {
        s_buttons_held &= ~button;
    }
}

void gbc_input_poll(void) {
}

void gbc_input_process_wsc_keys(uint16_t wsc_keys) {
    gbc_input_set_button(GB_BTN_UP,     (wsc_keys & WSC_KEY_X1) != 0);
    gbc_input_set_button(GB_BTN_RIGHT,  (wsc_keys & WSC_KEY_X2) != 0);
    gbc_input_set_button(GB_BTN_DOWN,   (wsc_keys & WSC_KEY_X3) != 0);
    gbc_input_set_button(GB_BTN_LEFT,   (wsc_keys & WSC_KEY_X4) != 0);

    gbc_input_set_button(GB_BTN_A,      (wsc_keys & WSC_KEY_A) != 0);
    gbc_input_set_button(GB_BTN_B,      (wsc_keys & WSC_KEY_B) != 0);
    gbc_input_set_button(GB_BTN_START,  (wsc_keys & WSC_KEY_START) != 0);
    gbc_input_set_button(GB_BTN_SELECT, (wsc_keys & WSC_KEY_Y1) != 0);

    uint16_t just_pressed = wsc_keys & ~s_prev_wsc_keys;
    if (just_pressed & WSC_KEY_Y4) {
        gbc_screen_mode_t current = gbc_video_get_screen_mode();
        gbc_video_set_screen_mode(current == GBC_SCREEN_MODE_STANDARD 
                                  ? GBC_SCREEN_MODE_WIDESCREEN 
                                  : GBC_SCREEN_MODE_STANDARD);
    }

    s_prev_wsc_keys = wsc_keys;
}

uint8_t gbc_input_read_p1(uint8_t p1_select) {
    uint8_t res = 0x0F;

    if (!(p1_select & 0x10)) {
        if (s_buttons_held & GB_BTN_RIGHT) res &= ~0x01;
        if (s_buttons_held & GB_BTN_LEFT)  res &= ~0x02;
        if (s_buttons_held & GB_BTN_UP)    res &= ~0x04;
        if (s_buttons_held & GB_BTN_DOWN)  res &= ~0x08;
    }

    if (!(p1_select & 0x20)) {
        if (s_buttons_held & GB_BTN_A)      res &= ~0x01;
        if (s_buttons_held & GB_BTN_B)      res &= ~0x02;
        if (s_buttons_held & GB_BTN_SELECT) res &= ~0x04;
        if (s_buttons_held & GB_BTN_START)  res &= ~0x08;
    }

    return (p1_select & 0x30) | res | 0xC0;
}
