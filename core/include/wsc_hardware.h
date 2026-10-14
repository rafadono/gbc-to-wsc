#ifndef WSC_HARDWARE_H
#define WSC_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

#define WSC_DISP_CTRL_PORT      0x00
#define WSC_DISP_MODE_PORT      0x60
#define WSC_SCR_BASE_PORT       0x07
#define WSC_SPR_BASE_PORT       0x04
#define WSC_SPR_FIRST_PORT      0x05
#define WSC_SPR_COUNT_PORT      0x06

#define WSC_SCR1_SCROLL_X_PORT  0x10
#define WSC_SCR1_SCROLL_Y_PORT  0x11
#define WSC_SCR2_SCROLL_X_PORT  0x12
#define WSC_SCR2_SCROLL_Y_PORT  0x13

#define WSC_LCD_LINE_PORT       0x14
#define WSC_LCD_INT_LINE_PORT   0x15

#define WSC_KEY_PORT            0xB5

#define WSC_DISP_CTRL_SCR1_ON   0x0001
#define WSC_DISP_CTRL_SCR2_ON   0x0002
#define WSC_DISP_CTRL_SPR_ON    0x0004
#define WSC_DISP_CTRL_WIN_ON    0x0008

#define WSC_DISP_MODE_COLOR     0x0080
#define WSC_DISP_MODE_4BPP      0x0040

#define WSC_KEY_X1              0x0001
#define WSC_KEY_X2              0x0002
#define WSC_KEY_X3              0x0004
#define WSC_KEY_X4              0x0008
#define WSC_KEY_A               0x0010
#define WSC_KEY_B               0x0020
#define WSC_KEY_START           0x0040
#define WSC_KEY_Y1              0x0100
#define WSC_KEY_Y2              0x0200
#define WSC_KEY_Y3              0x0400
#define WSC_KEY_Y4              0x0800

#define WSC_SPR_ATTR_VFLIP      0x80
#define WSC_SPR_ATTR_HFLIP      0x40
#define WSC_SPR_ATTR_PRIO_SCR2  0x20
#define WSC_SPR_ATTR_PRIO_SCR1  0x10
#define WSC_SPR_ATTR_PAL_MASK   0x0F

#define WSC_WIN_SCR2_X1_PORT    0x30
#define WSC_WIN_SCR2_Y1_PORT    0x31
#define WSC_WIN_SCR2_X2_PORT    0x32
#define WSC_WIN_SCR2_Y2_PORT    0x33

typedef struct {
    uint8_t tile;
    uint8_t flags;
    uint8_t x;
    uint8_t y;
} wsc_sprite_t;

typedef uint16_t wsc_color_t;

static inline wsc_color_t gbc_to_wsc_color(uint16_t bgr555) {
    uint16_t r = (bgr555 & 0x001F) >> 1;
    uint16_t g = ((bgr555 >> 5) & 0x001F) >> 1;
    uint16_t b = ((bgr555 >> 10) & 0x001F) >> 1;
    return (r << 8) | (g << 4) | b;
}

#endif
