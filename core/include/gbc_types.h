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
#define GB_REG_NR11     0xFF11
#define GB_REG_NR12     0xFF12
#define GB_REG_NR13     0xFF13
#define GB_REG_NR14     0xFF14
#define GB_REG_NR21     0xFF16
#define GB_REG_NR22     0xFF17
#define GB_REG_NR23     0xFF18
#define GB_REG_NR24     0xFF19
#define GB_REG_NR30     0xFF1A
#define GB_REG_NR31     0xFF1B
#define GB_REG_NR32     0xFF1C
#define GB_REG_NR33     0xFF1D
#define GB_REG_NR34     0xFF1E
#define GB_REG_NR41     0xFF20
#define GB_REG_NR42     0xFF21
#define GB_REG_NR43     0xFF22
#define GB_REG_NR44     0xFF23
#define GB_REG_NR50     0xFF24
#define GB_REG_NR51     0xFF25
#define GB_REG_NR52     0xFF26
#define GB_REG_WAVE_START 0xFF30
#define GB_REG_WAVE_END   0xFF3F
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
#define GB_REG_HDMA2    0xFF52
#define GB_REG_HDMA3    0xFF53
#define GB_REG_HDMA4    0xFF54
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

typedef enum {
    GBC_ORIENTATION_HORIZONTAL = 0,
    GBC_ORIENTATION_VERTICAL = 1
} gbc_orientation_t;

typedef enum {
    /* Hardware Clasico */
    DMG_PALETTE_ORIGINAL_GREEN = 0,
    DMG_PALETTE_POCKET_GRAYSCALE = 1,
    DMG_PALETTE_LIGHT_TEAL = 2,
    DMG_PALETTE_VIRTUAL_BOY = 3,

    /* GBC Boot ROM (12 combinaciones oficiales) */
    DMG_PALETTE_GBC_BROWN = 4,
    DMG_PALETTE_GBC_RED = 5,
    DMG_PALETTE_GBC_DARK_BROWN = 6,
    DMG_PALETTE_GBC_PASTEL = 7,
    DMG_PALETTE_GBC_ORANGE = 8,
    DMG_PALETTE_GBC_YELLOW = 9,
    DMG_PALETTE_GBC_BLUE = 10,
    DMG_PALETTE_GBC_DARK_BLUE = 11,
    DMG_PALETTE_GBC_GRAYSCALE = 12,
    DMG_PALETTE_GBC_GREEN = 13,
    DMG_PALETTE_GBC_DARK_GREEN = 14,
    DMG_PALETTE_GBC_INVERTED = 15,

    /* Super Game Boy */
    DMG_PALETTE_SGB_1A = 16,
    DMG_PALETTE_SGB_2A = 17,
    DMG_PALETTE_SGB_4G = 18,

    /* Comunidad (RetroArch, Lospec, Shaders) */
    DMG_PALETTE_WARM_SEPIA = 19,
    DMG_PALETTE_COOL_BLUE = 20,
    DMG_PALETTE_CRT_AMBER = 21,
    DMG_PALETTE_CRT_GREEN = 22,
    DMG_PALETTE_CYBERPUNK_NEON = 23,
    DMG_PALETTE_FROSTBITE = 24,
    DMG_PALETTE_CRIMSON_NIGHT = 25,
    DMG_PALETTE_WSC_VIBRANT = 26,

    DMG_PALETTE_COUNT = 27
} dmg_palette_preset_t;

typedef struct {
    bool enable_widescreen_toggle;
    bool enable_dmg_palette_cycle;
    bool enable_tate_mode;
    bool enable_enhanced_audio;
    dmg_palette_preset_t current_dmg_palette;
    gbc_orientation_t orientation;
} gbc_features_t;

extern gbc_features_t g_features;

#endif
