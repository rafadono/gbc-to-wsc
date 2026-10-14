#include <string.h>
#include "gbc_memory.h"
#include "gbc_video.h"
#include "gbc_input.h"
#include "gbc_audio.h"
#include "gbc_hal.h"

gb_state_t g_gb;

void gbc_memory_init(gb_state_t *gb, const uint8_t *rom, uint32_t rom_size, uint8_t *sram, uint32_t sram_size) {
    memset(gb, 0, sizeof(gb_state_t));

    gb->rom_bank_0 = rom;
    gb->rom_bank_n = (rom && rom_size >= 0x8000) ? (rom + 0x4000) : rom;
    gb->rom_size = rom_size;

    gb->sram = sram;
    gb->sram_size = sram_size;
    gb->sram_enabled = false;

    if (rom && rom_size >= 0x0150) {
        uint8_t cart_type = rom[0x0147];
        if (cart_type >= 0x01 && cart_type <= 0x03) {
            gb->mbc_type = MBC1;
        } else if (cart_type >= 0x19 && cart_type <= 0x1E) {
            gb->mbc_type = MBC5;
        } else if (cart_type >= 0x0F && cart_type <= 0x13) {
            gb->mbc_type = MBC3;
        } else if (cart_type == 0x05 || cart_type == 0x06) {
            gb->mbc_type = MBC2;
        } else {
            gb->mbc_type = MBC_NONE;
        }
        gb->is_cgb = (rom[0x0143] == 0x80 || rom[0x0143] == 0xC0);
    } else {
        gb->mbc_type = MBC1;
    }

    gb->current_rom_bank = 1;
    gb->current_wram_bank = 1;
    gb->current_vram_bank = 0;
    gb->current_sram_bank = 0;

    gb->joypad_state = 0xFF;

    gbc_memory_reset(gb);
}

void gbc_memory_reset(gb_state_t *gb) {
    memset(gb->wram, 0, sizeof(gb->wram));
    memset(gb->vram, 0, sizeof(gb->vram));
    memset(gb->hram, 0, sizeof(gb->hram));
    memset(gb->oam, 0, sizeof(gb->oam));
    memset(gb->io, 0, sizeof(gb->io));

    memset(gb->mbc2_ram, 0, sizeof(gb->mbc2_ram));
    gb->mbc3_rtc_mapped = false;
    gb->mbc3_rtc_reg_select = 0;
    memset(gb->mbc3_rtc_regs, 0, sizeof(gb->mbc3_rtc_regs));
    memset(gb->mbc3_rtc_latched, 0, sizeof(gb->mbc3_rtc_latched));
    gb->mbc3_latch_state = 0;

    gb->mbc1_mode = 0;
    gb->mbc1_low5 = 1;
    gb->mbc1_high2 = 0;
    gb->mbc5_rom_bank = 1;
    gb->current_rom_bank = 1;

    gb->io[GB_REG_LCDC & 0x7F] = 0x91;
    gb->io[GB_REG_BGP & 0x7F]  = 0xFC;
    gb->io[GB_REG_OBP0 & 0x7F] = 0xFF;
    gb->io[GB_REG_OBP1 & 0x7F] = 0xFF;

    gb->ime = 0;
    gb->ie = 0;
    gb->if_reg = 0;
    gb->div_counter = 0;
    gb->tima = 0;
    gb->tma = 0;
    gb->tac = 0;

    gbc_audio_reset();
}

uint8_t gbc_read8(uint16_t addr) {
    if (addr < 0x4000) {
        if (g_gb.rom_bank_0) {
            return g_gb.rom_bank_0[addr];
        }
        return 0xFF;
    }

    if (addr < 0x8000) {
        if (g_gb.rom_bank_n) {
            return g_gb.rom_bank_n[addr - 0x4000];
        }
        return 0xFF;
    }

    if (addr < 0xA000) {
        uint16_t offset = addr - 0x8000;
        if (g_gb.current_vram_bank == 1 && g_gb.is_cgb) {
            offset += 0x2000;
        }
        return g_gb.vram[offset];
    }

    if (addr < 0xC000) {
        if (g_gb.mbc_type == MBC2) {
            if (g_gb.sram_enabled) {
                return (g_gb.mbc2_ram[addr & 0x01FF] & 0x0F) | 0xF0;
            }
            return 0xFF;
        }
        if (g_gb.mbc_type == MBC3 && g_gb.mbc3_rtc_mapped) {
            if (g_gb.sram_enabled) {
                uint8_t r = g_gb.mbc3_rtc_reg_select;
                if (r >= 0x08 && r <= 0x0C) {
                    return g_gb.mbc3_rtc_latched[r - 0x08];
                }
            }
            return 0xFF;
        }
        if (g_gb.sram && g_gb.sram_enabled) {
            uint32_t sram_offset = (g_gb.current_sram_bank * 0x2000) + (addr - 0xA000);
            if (sram_offset < g_gb.sram_size) {
                return g_gb.sram[sram_offset];
            }
        }
        return 0xFF;
    }

    if (addr < 0xD000) {
        return g_gb.wram[addr - 0xC000];
    }

    if (addr < 0xE000) {
        uint32_t bank_offset = (uint32_t)g_gb.current_wram_bank * 0x1000;
        return g_gb.wram[bank_offset + (addr - 0xD000)];
    }

    if (addr < 0xFE00) {
        return gbc_read8(addr - 0x2000);
    }

    if (addr < 0xFEA0) {
        return g_gb.oam[addr - 0xFE00];
    }

    if (addr < 0xFF00) {
        return 0xFF;
    }

    if (addr >= GB_REG_NR10 && addr <= GB_REG_NR52) {
        return gbc_audio_read_reg(addr);
    }

    if (addr >= GB_REG_WAVE_START && addr <= GB_REG_WAVE_END) {
        return gbc_audio_read_reg(addr);
    }

    if (addr < 0xFF80) {
        uint8_t reg = addr & 0x7F;
        switch (addr) {
            case GB_REG_P1:
                return gbc_input_read_p1(g_gb.io[GB_REG_P1 & 0x7F]);
            case GB_REG_DIV:
                return (uint8_t)(g_gb.div_counter >> 8);
            case GB_REG_TIMA:
                return g_gb.tima;
            case GB_REG_TMA:
                return g_gb.tma;
            case GB_REG_TAC:
                return g_gb.tac | 0xF8;
            case GB_REG_IF:
                return g_gb.if_reg | 0xE0;
            case GB_REG_LY:
                return g_video.ly;
            case GB_REG_STAT:
                return g_video.stat | 0x80;
            case GB_REG_LCDC:
                return g_video.lcdc;
            case GB_REG_VBK:
                return g_gb.current_vram_bank | 0xFE;
            case GB_REG_SVBK:
                return g_gb.current_wram_bank | 0xF8;
            case GB_REG_HDMA5:
                return gbc_video_read_hdma5();
            case GB_REG_BCPS:
                return gbc_video_read_bcps();
            case GB_REG_BCPD:
                return gbc_video_read_bcpd();
            case GB_REG_OCPS:
                return gbc_video_read_ocps();
            case GB_REG_OCPD:
                return gbc_video_read_ocpd();
            default:
                return g_gb.io[reg];
        }
    }

    if (addr < 0xFFFF) {
        return g_gb.hram[addr - 0xFF80];
    }

    return g_gb.ie;
}

static void gbc_update_rom_bank_pointer(void) {
    if (!g_gb.rom_bank_0 || g_gb.rom_size < 0x8000) {
        return;
    }
    uint32_t total_banks = g_gb.rom_size / 0x4000;
    uint32_t bank_idx = g_gb.current_rom_bank;
    if (total_banks > 0) {
        bank_idx %= total_banks;
    }
    g_gb.rom_bank_n = g_gb.rom_bank_0 + (bank_idx * 0x4000);
}

void gbc_write8(uint16_t addr, uint8_t val) {
    if (addr < 0x8000) {
        if (g_gb.mbc_type == MBC1) {
            if (addr < 0x2000) {
                g_gb.sram_enabled = ((val & 0x0F) == 0x0A);
            } else if (addr < 0x4000) {
                uint8_t low = val & 0x1F;
                if (low == 0) low = 1;
                g_gb.mbc1_low5 = low;
                g_gb.current_rom_bank = (g_gb.mbc1_high2 << 5) | g_gb.mbc1_low5;
                gbc_update_rom_bank_pointer();
            } else if (addr < 0x6000) {
                uint8_t high = val & 0x03;
                g_gb.mbc1_high2 = high;
                if (g_gb.mbc1_mode == 0) {
                    g_gb.current_rom_bank = (high << 5) | g_gb.mbc1_low5;
                    gbc_update_rom_bank_pointer();
                } else {
                    g_gb.current_sram_bank = high;
                }
            } else {
                g_gb.mbc1_mode = val & 0x01;
                if (g_gb.mbc1_mode == 0) {
                    g_gb.current_sram_bank = 0;
                    g_gb.current_rom_bank = (g_gb.mbc1_high2 << 5) | g_gb.mbc1_low5;
                } else {
                    g_gb.current_rom_bank = g_gb.mbc1_low5;
                    g_gb.current_sram_bank = g_gb.mbc1_high2;
                }
                gbc_update_rom_bank_pointer();
            }
        } else if (g_gb.mbc_type == MBC2) {
            if (addr < 0x4000) {
                if (!(addr & 0x0100)) {
                    g_gb.sram_enabled = ((val & 0x0F) == 0x0A);
                } else {
                    uint8_t bank = val & 0x0F;
                    if (bank == 0) bank = 1;
                    g_gb.current_rom_bank = bank;
                    gbc_update_rom_bank_pointer();
                }
            }
        } else if (g_gb.mbc_type == MBC3) {
            if (addr < 0x2000) {
                g_gb.sram_enabled = ((val & 0x0F) == 0x0A);
            } else if (addr < 0x4000) {
                uint8_t bank = val & 0x7F;
                if (bank == 0) bank = 1;
                g_gb.current_rom_bank = bank;
                gbc_update_rom_bank_pointer();
            } else if (addr < 0x6000) {
                if (val <= 0x03) {
                    g_gb.current_sram_bank = val;
                    g_gb.mbc3_rtc_mapped = false;
                } else if (val >= 0x08 && val <= 0x0C) {
                    g_gb.mbc3_rtc_reg_select = val;
                    g_gb.mbc3_rtc_mapped = true;
                }
            } else {
                if (g_gb.mbc3_latch_state == 0 && val == 1) {
                    for (int i = 0; i < 5; i++) {
                        g_gb.mbc3_rtc_latched[i] = g_gb.mbc3_rtc_regs[i];
                    }
                }
                g_gb.mbc3_latch_state = val;
            }
        } else if (g_gb.mbc_type == MBC5) {
            if (addr < 0x2000) {
                g_gb.sram_enabled = ((val & 0x0F) == 0x0A);
            } else if (addr < 0x3000) {
                g_gb.mbc5_rom_bank = (g_gb.mbc5_rom_bank & 0x0100) | val;
                g_gb.current_rom_bank = g_gb.mbc5_rom_bank;
                gbc_update_rom_bank_pointer();
            } else if (addr < 0x4000) {
                g_gb.mbc5_rom_bank = (g_gb.mbc5_rom_bank & 0x00FF) | ((uint16_t)(val & 0x01) << 8);
                g_gb.current_rom_bank = g_gb.mbc5_rom_bank;
                gbc_update_rom_bank_pointer();
            } else if (addr < 0x6000) {
                g_gb.current_sram_bank = val & 0x0F;
            }
        }
        return;
    }

    if (addr < 0xA000) {
        uint16_t offset = addr - 0x8000;
        if (g_gb.current_vram_bank == 1 && g_gb.is_cgb) {
            offset += 0x2000;
        }
        g_gb.vram[offset] = val;

        if (offset < 0x1800) {
            gbc_video_mark_tile_dirty(offset >> 4);
        }
        return;
    }

    if (addr < 0xC000) {
        if (g_gb.mbc_type == MBC2) {
            if (g_gb.sram_enabled) {
                g_gb.mbc2_ram[addr & 0x01FF] = val & 0x0F;
            }
            return;
        }
        if (g_gb.mbc_type == MBC3 && g_gb.mbc3_rtc_mapped) {
            if (g_gb.sram_enabled) {
                uint8_t r = g_gb.mbc3_rtc_reg_select;
                if (r >= 0x08 && r <= 0x0C) {
                    g_gb.mbc3_rtc_regs[r - 0x08] = val;
                }
            }
            return;
        }
        if (g_gb.sram && g_gb.sram_enabled) {
            uint32_t sram_offset = (g_gb.current_sram_bank * 0x2000) + (addr - 0xA000);
            if (sram_offset < g_gb.sram_size) {
                g_gb.sram[sram_offset] = val;
            }
        }
        return;
    }

    if (addr < 0xD000) {
        g_gb.wram[addr - 0xC000] = val;
        return;
    }

    if (addr < 0xE000) {
        uint32_t bank_offset = (uint32_t)g_gb.current_wram_bank * 0x1000;
        g_gb.wram[bank_offset + (addr - 0xD000)] = val;
        return;
    }

    if (addr < 0xFE00) {
        gbc_write8(addr - 0x2000, val);
        return;
    }

    if (addr < 0xFEA0) {
        g_gb.oam[addr - 0xFE00] = val;
        return;
    }

    if (addr < 0xFF00) {
        return;
    }

    if (addr >= GB_REG_NR10 && addr <= GB_REG_NR52) {
        gbc_audio_write_reg(addr, val);
    }

    if (addr >= GB_REG_WAVE_START && addr <= GB_REG_WAVE_END) {
        gbc_audio_write_reg(addr, val);
    }

    if (addr < 0xFF80) {
        uint8_t reg = addr & 0x7F;
        g_gb.io[reg] = val;

        switch (addr) {
            case GB_REG_DIV:
                g_gb.div_counter = 0;
                break;
            case GB_REG_TIMA:
                g_gb.tima = val;
                break;
            case GB_REG_TMA:
                g_gb.tma = val;
                break;
            case GB_REG_TAC:
                g_gb.tac = val & 0x07;
                break;
            case GB_REG_IF:
                g_gb.if_reg = val & 0x1F;
                break;
            case GB_REG_DMA:
                gbc_dma_transfer(val);
                break;
            case GB_REG_LCDC:
                g_video.lcdc = val;
                break;
            case GB_REG_STAT:
                g_video.stat = (g_video.stat & 0x07) | (val & 0x78);
                break;
            case GB_REG_SCY:
                g_video.scy = val;
                break;
            case GB_REG_SCX:
                g_video.scx = val;
                break;
            case GB_REG_LYC:
                g_video.lyc = val;
                break;
            case GB_REG_BGP:
                g_video.bgp = val;
                gbc_video_update_palettes();
                break;
            case GB_REG_OBP0:
                g_video.obp0 = val;
                gbc_video_update_palettes();
                break;
            case GB_REG_OBP1:
                g_video.obp1 = val;
                gbc_video_update_palettes();
                break;
            case GB_REG_WY:
                g_video.wy = val;
                break;
            case GB_REG_WX:
                g_video.wx = val;
                break;
            case GB_REG_VBK:
                g_gb.current_vram_bank = val & 0x01;
                break;
            case GB_REG_SVBK: {
                uint8_t b = val & 0x07;
                if (b == 0) b = 1;
                g_gb.current_wram_bank = b;
                break;
            }
            case GB_REG_HDMA1:
                gbc_video_write_hdma1(val);
                break;
            case GB_REG_HDMA2:
                gbc_video_write_hdma2(val);
                break;
            case GB_REG_HDMA3:
                gbc_video_write_hdma3(val);
                break;
            case GB_REG_HDMA4:
                gbc_video_write_hdma4(val);
                break;
            case GB_REG_HDMA5:
                gbc_video_write_hdma5(val);
                break;
            case GB_REG_BCPS:
                gbc_video_write_bcps(val);
                break;
            case GB_REG_BCPD:
                gbc_video_write_bcpd(val);
                break;
            case GB_REG_OCPS:
                gbc_video_write_ocps(val);
                break;
            case GB_REG_OCPD:
                gbc_video_write_ocpd(val);
                break;
        }
        return;
    }

    if (addr < 0xFFFF) {
        g_gb.hram[addr - 0xFF80] = val;
        return;
    }

    g_gb.ie = val & 0x1F;
}

uint16_t gbc_read16(uint16_t addr) {
    uint8_t l = gbc_read8(addr);
    uint8_t h = gbc_read8(addr + 1);
    return (uint16_t)l | ((uint16_t)h << 8);
}

void gbc_write16(uint16_t addr, uint16_t val) {
    gbc_write8(addr, (uint8_t)(val & 0xFF));
    gbc_write8(addr + 1, (uint8_t)(val >> 8));
}

void gbc_dma_transfer(uint8_t base_page) {
    uint16_t src = (uint16_t)base_page << 8;
    for (uint16_t i = 0; i < 0xA0; i++) {
        g_gb.oam[i] = gbc_read8(src + i);
    }
}
