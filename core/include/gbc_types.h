#ifndef GBC_TYPES_H
#define GBC_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define GB_SCREEN_W 160
#define GB_SCREEN_H 144
#define WSC_SCREEN_W 224
#define WSC_SCREEN_H 144

#define GB_VIEWPORT_X_OFFSET 32

#define GB_REG_P1       0xFF00
#define GB_REG_SB       0xFF01
#define GB_REG_SC       0xFF02
#define GB_REG_DIV      0xFF04
#define GB_REG_TIMA     0xFF05
#define GB_REG_TMA      0xFF06
#define GB_REG_TAC      0xFF07
#define GB_REG_IF       0xFF0F
#define GB_REG_NR10     0xFF10
#define GB_REG_NR52     0xFF26
#define GB_REG_LCDC     0xFF40
#define GB_REG_STAT     0xFF41
#define GB_REG_SCY      0xFF42
#define GB_REG_SCX      0xFF43
#define GB_REG_LY       0xFF44
#define GB_REG_LYC      0xFF45
#define GB_REG_DMA      0xFF46
#define GB_REG_BGP      0xFF47
#define GB_REG_OBP0     0xFF48
#define GB_REG_OBP1     0xFF49
#define GB_REG_WY       0xFF4A
#define GB_REG_WX       0xFF4B
#define GB_REG_KEY1     0xFF4D
#define GB_REG_VBK      0xFF4F
#define GB_REG_HDMA1    0xFF51
#define GB_REG_HDMA5    0xFF55
#define GB_REG_BCPS     0xFF68
#define GB_REG_BCPD     0xFF69
#define GB_REG_OCPS     0xFF6A
#define GB_REG_OCPD     0xFF6B
#define GB_REG_SVBK     0xFF70
#define GB_REG_IE       0xFFFF

#define GB_LCDC_ENABLE          0x80
#define GB_LCDC_WIN_TILEMAP     0x40
#define GB_LCDC_WIN_ENABLE      0x20
#define GB_LCDC_BG_WIN_TILEDATA 0x10
#define GB_LCDC_BG_TILEMAP      0x08
#define GB_LCDC_OBJ_SIZE        0x04
#define GB_LCDC_OBJ_ENABLE      0x02
#define GB_LCDC_BG_PRIORITY     0x01

#define GB_STAT_LYC_INT         0x40
#define GB_STAT_OAM_INT         0x20
#define GB_STAT_VBLANK_INT      0x10
#define GB_STAT_HBLANK_INT      0x08
#define GB_STAT_LYC_FLAG        0x04
#define GB_STAT_MODE_MASK       0x03

#define GB_INT_VBLANK           0x01
#define GB_INT_LCD_STAT         0x02
#define GB_INT_TIMER            0x04
#define GB_INT_SERIAL           0x08
#define GB_INT_JOYPAD           0x10

#define GB_BTN_RIGHT            0x01
#define GB_BTN_LEFT             0x02
#define GB_BTN_UP               0x04
#define GB_BTN_DOWN             0x08
#define GB_BTN_A                0x10
#define GB_BTN_B                0x20
#define GB_BTN_SELECT           0x40
#define GB_BTN_START            0x80

typedef struct {
    uint8_t y;
    uint8_t x;
    uint8_t tile;
    uint8_t flags;
} gb_oam_entry_t;

#endif
