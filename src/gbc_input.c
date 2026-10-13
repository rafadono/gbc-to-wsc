#include "gbc_input.h"
#include "gbc_memory.h"
#include "gbc_hal.h"

static uint8_t s_buttons_held = 0;

void gbc_input_init(void) {
    s_buttons_held = 0;
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
