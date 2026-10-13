#ifndef GBC_INPUT_H
#define GBC_INPUT_H

#include "gbc_types.h"

void gbc_input_init(void);
void gbc_input_poll(void);

uint8_t gbc_input_read_p1(uint8_t p1_select);
void gbc_input_set_button(uint8_t button, bool pressed);

#endif
