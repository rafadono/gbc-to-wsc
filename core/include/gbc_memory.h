#ifndef GBC_MEMORY_H
#define GBC_MEMORY_H

#include "gbc_types.h"

#define GBC_WRAM_SIZE       0x8000
#define GBC_VRAM_SIZE       0x4000
#define GBC_HRAM_SIZE       0x0080
#define GBC_OAM_SIZE        0x00A0
#define GBC_IO_SIZE         0x0080

typedef enum {
    MBC_NONE = 0,
    MBC1,
    MBC2,
    MBC3,
    MBC5
} mbc_type_t;

typedef struct {
    uint8_t wram[GBC_WRAM_SIZE];
    uint8_t vram[GBC_VRAM_SIZE];
    uint8_t hram[GBC_HRAM_SIZE];
    uint8_t oam[GBC_OAM_SIZE];
    uint8_t io[GBC_IO_SIZE];

    const uint8_t *rom_bank_0;
    const uint8_t *rom_bank_n;
    uint32_t rom_size;

    uint8_t *sram;
    uint32_t sram_size;
    bool sram_enabled;

    mbc_type_t mbc_type;
    uint8_t mbc1_mode;
    uint8_t mbc1_low5;
    uint8_t mbc1_high2;
    uint16_t mbc5_rom_bank;

    uint8_t mbc2_ram[512];
    bool mbc3_rtc_mapped;
    uint8_t mbc3_rtc_reg_select;
    uint8_t mbc3_rtc_regs[5];
    uint8_t mbc3_rtc_latched[5];
    uint8_t mbc3_latch_state;

    uint16_t current_rom_bank;
    uint8_t current_wram_bank;
    uint8_t current_vram_bank;
    uint8_t current_sram_bank;

    uint8_t ime;
    uint8_t ie;
    uint8_t if_reg;

    uint16_t div_counter;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;

    uint8_t joypad_state;

    bool is_cgb;
    uint8_t world_level_buffer[16384];
} gb_state_t;

extern gb_state_t g_gb;

void gbc_memory_init(gb_state_t *gb, const uint8_t *rom, uint32_t rom_size, uint8_t *sram, uint32_t sram_size);
void gbc_memory_reset(gb_state_t *gb);

uint8_t gbc_read8(uint16_t addr);
void gbc_write8(uint16_t addr, uint8_t val);

uint16_t gbc_read16(uint16_t addr);
void gbc_write16(uint16_t addr, uint16_t val);

void gbc_dma_transfer(uint8_t base_page);

void gbc_memory_load_level(const uint8_t *src_level_data, size_t size);
uint8_t gbc_memory_read_level_tile(uint16_t x, uint16_t y, uint16_t stride);
void gbc_memory_write_level_tile(uint16_t x, uint16_t y, uint16_t stride, uint8_t tile);
uint8_t *gbc_memory_get_level_buffer(void);

#endif
