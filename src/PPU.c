// PPU.c
// Nintendo Entertainment System PPU Implementation (2C02)
// Will Holbrook - 1st December 2024

#include "PPU.h"
#include "Cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>


// NES Palette (Colour)
static const uint32_t NES_PALETTE[64] = {
    0x626262, 0x002A93, 0x0000BC, 0x3E00BC, 0x6C0093, 0x86094C, 0x860C06, 0x6C1D06,
    0x3C3609, 0x004F0F, 0x005B13, 0x005913, 0x00484D, 0x000000, 0x000000, 0x000000,
    0xABABAB, 0x0060ED, 0x2F34F7, 0x7C09F7, 0xB901ED, 0xDD1C8C, 0xDD2E24, 0xB94B16,
    0x77701C, 0x009024, 0x00A029, 0x009E32, 0x00878E, 0x000000, 0x000000, 0x000000,
    0xFFFFFF, 0x03B3FA, 0x838AF9, 0xD269F8, 0xFF5AF8, 0xFF5EDD, 0xFF776D, 0xFF9A30,
    0xCAC238, 0x6DE340, 0x00F445, 0x00F179, 0x00D9E1, 0x4E4E4E, 0x000000, 0x000000,
    0xFFFFFF, 0xADE0FD, 0xCDD0FC, 0xEEC3FC, 0xFFBDFB, 0xFFBEF1, 0xFFC7C4, 0xFFD6A0,
    0xEAE78C, 0xC8F48D, 0xA8FBA3, 0x97F9C7, 0x99EFF3, 0xB8B8B8, 0x000000, 0x000000
};

static inline uint32_t get_palette_colour(uint8_t i) {
    // Ensure the palette index is within 0-63 and return an SDL-friendly 32-bit colour with full alpha.
    uint8_t index = i & 0x3F;
    return 0x000000FF | (NES_PALETTE[index] << 8);
}

// Flip a byte horizontally (used for sprite rendering)
static inline uint8_t flipbyte(uint8_t b) {
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
    return b;
}

// PPU Initialization & Reset

Ppu* init_ppu() {
    Ppu* ppu = (Ppu*)malloc(sizeof(Ppu));
    if (!ppu) {
        fprintf(stderr, "[PPU] Failed to allocate memory for PPU\n");
        exit(1);
    }

    ppu->cart = NULL;
    memset(ppu->framebuffer, 0, PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT * sizeof(uint32_t));
    memset(ppu->name_table, 0, sizeof(ppu->name_table));
    memset(ppu->palette_table, 0, sizeof(ppu->palette_table));
    memset(ppu->pattern_table, 0, sizeof(ppu->pattern_table));

    ppu->scanline = 0;
    ppu->cycle = 0;
    ppu->frames_completed = 0;

    ppu->registers.ctrl = (PpuCtrl){0};
    ppu->registers.status = (PpuStatus){0};
    ppu->registers.mask = (PpuMask){0};

    ppu->vram_addr = (LoopyRegister){0};
    ppu->vram_addr.reg = 0x0000;
    ppu->tram_addr = (LoopyRegister){0};
    ppu->tram_addr.reg = 0x0000;

    ppu->address_latch = 0x00;
    ppu->ppu_data_buffer = 0x00;
    ppu->fine_x = 0x00;
    ppu->bg_next_tile_id = 0x00;
    ppu->bg_next_tile_attr = 0x00;
    ppu->bg_next_tile_lsb = 0x00;
    ppu->bg_next_tile_msb = 0x00;
    ppu->bg_shifter_pattern_lo = 0x0000;
    ppu->bg_shifter_pattern_hi = 0x0000;
    ppu->bg_shifter_attrib_lo = 0x0000;
    ppu->bg_shifter_attrib_hi = 0x0000;

    ppu->oam_addr = 0x00;
    ppu->b_sprite_zero_hit_possible = false;
    ppu->b_sprite_zero_being_rendered = false;
    ppu->p_oam = (uint8_t*)ppu->oam;
    ppu->frame_done = false;
    ppu->nmi_occurred = false;

    return ppu;
}

void ppu_reset(Ppu* ppu) {
    memset(ppu->framebuffer, 0, PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT * sizeof(uint32_t));
    memset(ppu->name_table, 0, sizeof(ppu->name_table));
    memset(ppu->palette_table, 0, sizeof(ppu->palette_table));
    memset(ppu->pattern_table, 0, sizeof(ppu->pattern_table));

    ppu->scanline = 0;
    ppu->cycle = 0;
    ppu->frames_completed = 0;

    ppu->registers.ctrl = (PpuCtrl){0};
    ppu->registers.status = (PpuStatus){0};
    ppu->registers.mask = (PpuMask){0};

    ppu->vram_addr = (LoopyRegister){0};
    ppu->vram_addr.reg = 0x0000;
    ppu->tram_addr = (LoopyRegister){0};
    ppu->tram_addr.reg = 0x0000;

    ppu->address_latch = 0x00;
    ppu->ppu_data_buffer = 0x00;
    ppu->fine_x = 0x00;
    ppu->bg_next_tile_id = 0x00;
    ppu->bg_next_tile_attr = 0x00;
    ppu->bg_next_tile_lsb = 0x00;
    ppu->bg_next_tile_msb = 0x00;
    ppu->bg_shifter_pattern_lo = 0x0000;
    ppu->bg_shifter_pattern_hi = 0x0000;
    ppu->bg_shifter_attrib_lo = 0x0000;
    ppu->bg_shifter_attrib_hi = 0x0000;

    ppu->oam_addr = 0x00;
    ppu->b_sprite_zero_hit_possible = false;
    ppu->b_sprite_zero_being_rendered = false;
    ppu->p_oam = (uint8_t*)ppu->oam;
    ppu->frame_done = false;
    ppu->nmi_occurred = false;
}


// PPU Scrolling & Background Shifter Functions
void ppu_increment_scroll_x(Ppu* ppu) {
    if (ppu->registers.mask.render_background || ppu->registers.mask.render_sprites) {
        if (ppu->vram_addr.coarse_x == 31) {
            ppu->vram_addr.coarse_x = 0;
            ppu->vram_addr.nametable_x = ~ppu->vram_addr.nametable_x;
        } else {
            ppu->vram_addr.coarse_x++;
        }
    }
}

void ppu_increment_scroll_y(Ppu* ppu) {
    if (ppu->registers.mask.render_background || ppu->registers.mask.render_sprites) {
        if (ppu->vram_addr.fine_y < 7) {
            ppu->vram_addr.fine_y++;
        } else {
            ppu->vram_addr.fine_y = 0;
            if (ppu->vram_addr.coarse_y == 29) {
                ppu->vram_addr.coarse_y = 0;
                ppu->vram_addr.nametable_y = ~ppu->vram_addr.nametable_y;
            } else if (ppu->vram_addr.coarse_y == 31) {
                ppu->vram_addr.coarse_y = 0;
            } else {
                ppu->vram_addr.coarse_y++;
            }
        }
    }
}

void ppu_transfer_address_x(Ppu* ppu) {
    if (ppu->registers.mask.render_background || ppu->registers.mask.render_sprites) {
        ppu->vram_addr.nametable_x = ppu->tram_addr.nametable_x;
        ppu->vram_addr.coarse_x = ppu->tram_addr.coarse_x;
    }
}

void ppu_transfer_address_y(Ppu* ppu) {
    if (ppu->registers.mask.render_background || ppu->registers.mask.render_sprites) {
        ppu->vram_addr.fine_y = ppu->tram_addr.fine_y;
        ppu->vram_addr.nametable_y = ppu->tram_addr.nametable_y;
        ppu->vram_addr.coarse_y = ppu->tram_addr.coarse_y;
    }
}

void ppu_load_bg_shifters(Ppu* ppu) {
    ppu->bg_shifter_pattern_lo = (ppu->bg_shifter_pattern_lo & 0xFF00) | ppu->bg_next_tile_lsb;
    ppu->bg_shifter_pattern_hi = (ppu->bg_shifter_pattern_hi & 0xFF00) | ppu->bg_next_tile_msb;
    ppu->bg_shifter_attrib_lo = (ppu->bg_shifter_attrib_lo & 0xFF00) | ((ppu->bg_next_tile_attr & 0b01) ? 0xFF : 0x00);
    ppu->bg_shifter_attrib_hi = (ppu->bg_shifter_attrib_hi & 0xFF00) | ((ppu->bg_next_tile_attr & 0b10) ? 0xFF : 0x00);
}

void ppu_update_shifters(Ppu* ppu) {
    if (ppu->registers.mask.render_background) {
        ppu->bg_shifter_pattern_lo <<= 1;
        ppu->bg_shifter_pattern_hi <<= 1;
        ppu->bg_shifter_attrib_lo <<= 1;
        ppu->bg_shifter_attrib_hi <<= 1;
    }
    if (ppu->registers.mask.render_sprites && ppu->cycle >= 1 && ppu->cycle < 258) {
        for (size_t i = 0; i < ppu->sprite_count; i++) {
            if (ppu->sprite_scanline[i].x > 0) {
                ppu->sprite_scanline[i].x--;
            } else {
                ppu->sprite_shifter_pattern_lo[i] <<= 1;
                ppu->sprite_shifter_pattern_hi[i] <<= 1;
            }
        }
    }
}


// PPU Clock & Rendering Process
void ppu_clock(Ppu* ppu) {
    // Pre-render and visible scanlines
    if (ppu->scanline >= -1 && ppu->scanline < 240) {
        if (ppu->scanline == 0 && ppu->cycle == 0 &&
            (ppu->frames_completed % 2 != 0) &&
            (ppu->registers.mask.render_background || ppu->registers.mask.render_sprites)) {
            ppu->cycle = 1;
        }
        if (ppu->scanline == -1 && ppu->cycle == 1) {
            ppu->registers.status.vertical_blank = 0;
            ppu->registers.status.sprite_overflow = 0;
            ppu->registers.status.sprite_zero_hit = 0;
            for (size_t i = 0; i < 8; i++) {
                ppu->sprite_shifter_pattern_lo[i] = 0;
                ppu->sprite_shifter_pattern_hi[i] = 0;
            }
        }
        if ((ppu->cycle >= 2 && ppu->cycle < 258) ||
            (ppu->cycle >= 321 && ppu->cycle < 338)) {
            ppu_update_shifters(ppu);
            switch ((ppu->cycle - 1) % 8) {
                case 0:
                    ppu_load_bg_shifters(ppu);
                    ppu->bg_next_tile_id = ppu_read(ppu, 0x2000 | (ppu->vram_addr.reg & 0x0FFF));
                    break;
                case 2:
                    ppu->bg_next_tile_attr = ppu_read(ppu,
                        0x23C0 |
                        (ppu->vram_addr.nametable_y << 11) |
                        (ppu->vram_addr.nametable_x << 10) |
                        ((ppu->vram_addr.coarse_y >> 2) << 3) |
                        (ppu->vram_addr.coarse_x >> 2));
                    if (ppu->vram_addr.coarse_y & 0x02) ppu->bg_next_tile_attr >>= 4;
                    if (ppu->vram_addr.coarse_x & 0x02) ppu->bg_next_tile_attr >>= 2;
                    ppu->bg_next_tile_attr &= 0x03;
                    break;
                case 4:
                    ppu->bg_next_tile_lsb = ppu_read(ppu,
                        (ppu->registers.ctrl.pattern_background << 12) +
                        ((uint16_t)ppu->bg_next_tile_id << 4) +
                        ppu->vram_addr.fine_y);
                    break;
                case 6:
                    ppu->bg_next_tile_msb = ppu_read(ppu,
                        (ppu->registers.ctrl.pattern_background << 12) +
                        ((uint16_t)ppu->bg_next_tile_id << 4) +
                        ppu->vram_addr.fine_y + 8);
                    break;
                case 7:
                    ppu_increment_scroll_x(ppu);
                    break;
                default:
                    break;
            }
        }
        if (ppu->cycle == 256) {
            ppu_increment_scroll_y(ppu);
        }
        if (ppu->cycle == 257) {
            ppu_load_bg_shifters(ppu);
            ppu_transfer_address_x(ppu);
        }
        if (ppu->cycle == 338 || ppu->cycle == 340) {
            ppu->bg_next_tile_id = ppu_read(ppu, 0x2000 | (ppu->vram_addr.reg & 0x0FFF));
        }
        if (ppu->scanline == -1 && ppu->cycle >= 280 && ppu->cycle < 305) {
            ppu_transfer_address_y(ppu);
        }
    }

    // Sprite evaluation (cycle 257)
    if (ppu->cycle == 257 && ppu->scanline >= 0) {
        memset(ppu->sprite_scanline, 0xFF, 8 * sizeof(sObjectAttributeEntry));
        ppu->sprite_count = 0;
        for (uint8_t i = 0; i < 8; i++) {
            ppu->sprite_shifter_pattern_lo[i] = 0;
            ppu->sprite_shifter_pattern_hi[i] = 0;
        }
        uint8_t n_oam_entry = 0;
        ppu->b_sprite_zero_hit_possible = false;
        while (n_oam_entry < 64 && ppu->sprite_count < 9) {
            int16_t diff = ((int16_t)ppu->scanline - (int16_t)ppu->oam[n_oam_entry].y);
            if (diff >= 0 && diff < (ppu->registers.ctrl.sprite_size ? 16 : 8) && ppu->sprite_count < 8) {
                if (n_oam_entry == 0) {
                    ppu->b_sprite_zero_hit_possible = true;
                }
                memcpy(&ppu->sprite_scanline[ppu->sprite_count],
                       &ppu->oam[n_oam_entry],
                       sizeof(sObjectAttributeEntry));
                ppu->sprite_count++;
            }
            n_oam_entry++;
        }
        ppu->registers.status.sprite_overflow = (ppu->sprite_count >= 8);
    }

    // Sprite fetching (cycle 340)
    if (ppu->cycle == 340) {
        for (uint8_t i = 0; i < ppu->sprite_count; i++) {
            uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
            uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;
            if (!ppu->registers.ctrl.sprite_size) {
                // 8x8 Sprite Mode
                if (!(ppu->sprite_scanline[i].attribute & 0x80)) {
                    sprite_pattern_addr_lo = 
                        (ppu->registers.ctrl.pattern_sprite << 12) |
                        (ppu->sprite_scanline[i].id << 4) |
                        (ppu->scanline - ppu->sprite_scanline[i].y);
                } else {
                    sprite_pattern_addr_lo = 
                        (ppu->registers.ctrl.pattern_sprite << 12) |
                        (ppu->sprite_scanline[i].id << 4) |
                        (7 - (ppu->scanline - ppu->sprite_scanline[i].y));
                }
            } else {
                // 8x16 Sprite Mode
                if (!(ppu->sprite_scanline[i].attribute & 0x80)) {
                    if (ppu->scanline - ppu->sprite_scanline[i].y < 8) {
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12) |
                            ((ppu->sprite_scanline[i].id & 0xFE) << 4) |
                            ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
                    } else {
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12) |
                            (((ppu->sprite_scanline[i].id & 0xFE) + 1) << 4) |
                            ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
                    }
                } else {
                    if (ppu->scanline - ppu->sprite_scanline[i].y < 8) {
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12) |
                            (((ppu->sprite_scanline[i].id & 0xFE) + 1) << 4) |
                            (7 - ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07));
                    } else {
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12) |
                            ((ppu->sprite_scanline[i].id & 0xFE) << 4) |
                            (7 - ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07));
                    }
                }
            }
            sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;
            sprite_pattern_bits_lo = ppu_read(ppu, sprite_pattern_addr_lo);
            sprite_pattern_bits_hi = ppu_read(ppu, sprite_pattern_addr_hi);
            if (ppu->sprite_scanline[i].attribute & 0x40) {
                sprite_pattern_bits_lo = flipbyte(sprite_pattern_bits_lo);
                sprite_pattern_bits_hi = flipbyte(sprite_pattern_bits_hi);
            }
            ppu->sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
            ppu->sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
        }
    }

    // VBlank: scanlines 241-260
    if (ppu->scanline >= 241 && ppu->scanline < 261) {
        if (ppu->scanline == 241 && ppu->cycle == 1) {
            ppu->registers.status.vertical_blank = 1;
            if (ppu->registers.ctrl.enable_nmi) {
                ppu->nmi_occurred = true;
            }
        }
    }

    // Pixel rendering & framebuffer update
    uint8_t bg_pixel = 0x00, bg_palette = 0x00;
    if (ppu->registers.mask.render_background) {
        if (ppu->registers.mask.render_background_left || (ppu->cycle >= 9)) {
            uint16_t bit_mux = 0x8000 >> ppu->fine_x;
            uint8_t p0_pixel = (ppu->bg_shifter_pattern_lo & bit_mux) > 0;
            uint8_t p1_pixel = (ppu->bg_shifter_pattern_hi & bit_mux) > 0;
            bg_pixel = (p1_pixel << 1) | p0_pixel;
            uint8_t bg_palette0 = (ppu->bg_shifter_attrib_lo & bit_mux) > 0;
            uint8_t bg_palette1 = (ppu->bg_shifter_attrib_hi & bit_mux) > 0;
            bg_palette = (bg_palette1 << 1) | bg_palette0;
        }
    }
    uint8_t fg_pixel = 0x00, fg_palette = 0x00, fg_priority = 0x00;
    if (ppu->registers.mask.render_sprites) {
        if (ppu->registers.mask.render_sprites_left || (ppu->cycle >= 9)) {
            ppu->b_sprite_zero_being_rendered = false;
            for (uint8_t i = 0; i < ppu->sprite_count; i++) {
                if (ppu->sprite_scanline[i].x == 0) {
                    uint8_t fg_pixel_lo = (ppu->sprite_shifter_pattern_lo[i] & 0x80) > 0;
                    uint8_t fg_pixel_hi = (ppu->sprite_shifter_pattern_hi[i] & 0x80) > 0;
                    fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;
                    fg_palette = (ppu->sprite_scanline[i].attribute & 0x03) + 0x04;
                    fg_priority = (ppu->sprite_scanline[i].attribute & 0x20) == 0;
                    if (fg_pixel != 0) {
                        if (i == 0) {
                            ppu->b_sprite_zero_being_rendered = true;
                        }
                        break;
                    }
                }
            }
        }
    }
    uint8_t pixel = 0x00, palette = 0x00;
    if (bg_pixel == 0 && fg_pixel == 0) {
        pixel = 0x00;
        palette = 0x00;
    } else if (bg_pixel == 0 && fg_pixel > 0) {
        pixel = fg_pixel;
        palette = fg_palette;
    } else if (bg_pixel > 0 && fg_pixel == 0) {
        pixel = bg_pixel;
        palette = bg_palette;
    } else if (bg_pixel > 0 && fg_pixel > 0) {
        pixel = (fg_priority) ? fg_pixel : bg_pixel;
        palette = (fg_priority) ? fg_palette : bg_palette;
        if (ppu->b_sprite_zero_hit_possible && ppu->b_sprite_zero_being_rendered) {
            if ((ppu->registers.mask.render_background & ppu->registers.mask.render_sprites) &&
               (!(ppu->registers.mask.render_background_left | ppu->registers.mask.render_sprites_left))) {
                if (ppu->cycle >= 9 && ppu->cycle < 258) {
                    ppu->registers.status.sprite_zero_hit = 1;
                }
            } else {
                if (ppu->cycle >= 1 && ppu->cycle < 258) {
                    ppu->registers.status.sprite_zero_hit = 1;
                }
            }
        }
    }
    if ((ppu->cycle - 1) >= 0 && (ppu->cycle - 1) < PPU_SCREEN_WIDTH &&
        ppu->scanline >= 0 && ppu->scanline < PPU_SCREEN_HEIGHT) {
        ppu->framebuffer[ppu->scanline * PPU_SCREEN_WIDTH + (ppu->cycle - 1)] =
            get_palette_colour(ppu_read(ppu, 0x3F00 + (palette << 2) + pixel));
    }

    // Advance PPU cycle and update scanline/frame counters.
    ppu->cycle++;
    if (ppu->registers.mask.render_background || ppu->registers.mask.render_sprites) {
        if (ppu->cycle == 260 && ppu->scanline < 240) {
            // Mapper-specific scanline interrupt handling (if applicable)
        }
    }
    if (ppu->cycle >= 341) {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline >= 261) {
            ppu->scanline = -1;
            ppu->frame_done = true;
            ppu->frames_completed++;
        }
    }
}


// CPU 'Interface' for PPU Registers
uint8_t cpu_ppu_read(Ppu* ppu, uint16_t address) {
    uint8_t data = 0x00;
    switch (address) {
        case 0x0002: // Status
            data = (ppu->registers.status.reg & 0xE0) | (ppu->ppu_data_buffer & 0x1F);
            ppu->registers.status.vertical_blank = 0;
            ppu->address_latch = 0;
            break;
        case 0x0003: // OAM Address
            // not implemented
            break;
        case 0x0004: // OAM Data
            data = ppu->p_oam[ppu->oam_addr];
            break;
        case 0x0007: // PPU Data
            data = ppu->ppu_data_buffer;
            ppu->ppu_data_buffer = ppu_read(ppu, ppu->vram_addr.reg);
            if (ppu->vram_addr.reg >= 0x3F00) {
                data = ppu->ppu_data_buffer;
            }
            ppu->vram_addr.reg += ppu->registers.ctrl.increment_mode ? 32 : 1;
            break;
        default:
        // Invalid
        break;
    }
    return data;
}

void cpu_ppu_write(Ppu* ppu, uint16_t address, uint8_t data) {
    switch (address) {
        case 0x0000: // Control
            ppu->registers.ctrl.reg = data;
            ppu->tram_addr.nametable_x = ppu->registers.ctrl.nametable_x;
            ppu->tram_addr.nametable_y = ppu->registers.ctrl.nametable_y;
            break;
        case 0x0001: // Mask
            ppu->registers.mask.reg = data;
            break;
        case 0x0003: // OAM Address
            ppu->oam_addr = data;
            break;
        case 0x0004: // OAM Data
            ppu->p_oam[ppu->oam_addr] = data;
            break;
        case 0x0005: // Scroll
            if (ppu->address_latch == 0) {
                ppu->fine_x = data & 0x07;
                ppu->tram_addr.coarse_x = data >> 3;
                ppu->address_latch = 1;
            } else {
                ppu->tram_addr.fine_y = data & 0x07;
                ppu->tram_addr.coarse_y = data >> 3;
                ppu->address_latch = 0;
            }
            break;
        case 0x0006: // PPU Address
            if (ppu->address_latch == 0) {
                ppu->tram_addr.reg = ((data & 0x3F) << 8) | (ppu->tram_addr.reg & 0x00FF);
                ppu->address_latch = 1;
            } else {
                ppu->tram_addr.reg = (ppu->tram_addr.reg & 0xFF00) | data;
                ppu->vram_addr = ppu->tram_addr;
                ppu->address_latch = 0;
            }
            break;
        case 0x0007: // PPU Data
            ppu_write(ppu, ppu->vram_addr.reg, data);
            ppu->vram_addr.reg += ppu->registers.ctrl.increment_mode ? 32 : 1;
            break;
        default:
            // Invalid
            break;
    }
}

// Internal PPU Memory Access Functions
uint8_t ppu_read(Ppu* ppu, uint16_t address) {
    uint8_t data = 0x00;
    address &= 0x3FFF;
    if (cartridge_ppu_read(ppu->cart, address, &data)) {
        // Read handled by cartridge
    }
    else if (address <= 0x1FFF) { // Pattern table region
        data = ppu->pattern_table[(address & 0x1000) >> 12][address & 0x0FFF];
    }
    else if (address >= 0x2000 && address <= 0x3EFF) { // Name table region
        address &= 0x0FFF;
        if (ppu->cart->mirror == VERTICAL) {
            if (address <= 0x03FF) data = ppu->name_table[0][address & 0x03FF];
            else if (address <= 0x07FF) data = ppu->name_table[1][address & 0x03FF];
            else if (address <= 0x0BFF) data = ppu->name_table[0][address & 0x03FF];
            else if (address <= 0x0FFF) data = ppu->name_table[1][address & 0x03FF];
        }
        else if (ppu->cart->mirror == HORIZONTAL) {
            if (address <= 0x03FF) data = ppu->name_table[0][address & 0x03FF];
            else if (address <= 0x07FF) data = ppu->name_table[0][address & 0x03FF];
            else if (address <= 0x0BFF) data = ppu->name_table[1][address & 0x03FF];
            else if (address <= 0x0FFF) data = ppu->name_table[1][address & 0x03FF];
        }
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) { // Palette region
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        data = ppu->palette_table[address] & (ppu->registers.mask.grayscale ? 0x30 : 0x3F);
    }
    return data;
}

void ppu_write(Ppu* ppu, uint16_t address, uint8_t data) {
    address &= 0x3FFF;
    if (cartridge_ppu_write(ppu->cart, address, data)) {
        // Write handled by cartridge
    }
    else if (address <= 0x1FFF) { // Pattern table region
        ppu->pattern_table[(address & 0x1000) >> 12][address & 0x0FFF] = data;
    }
    else if (address >= 0x2000 && address <= 0x3EFF) { // Name table region
        address &= 0x0FFF;
        if (ppu->cart->mirror == VERTICAL) {
            if (address <= 0x03FF) ppu->name_table[0][address & 0x03FF] = data;
            else if (address <= 0x07FF) ppu->name_table[1][address & 0x03FF] = data;
            else if (address <= 0x0BFF) ppu->name_table[0][address & 0x03FF] = data;
            else if (address <= 0x0FFF) ppu->name_table[1][address & 0x03FF] = data;
        }
        else if (ppu->cart->mirror == HORIZONTAL) {
            if (address <= 0x03FF) ppu->name_table[0][address & 0x03FF] = data;
            else if (address <= 0x07FF) ppu->name_table[0][address & 0x03FF] = data;
            else if (address <= 0x0BFF) ppu->name_table[1][address & 0x03FF] = data;
            else if (address <= 0x0FFF) ppu->name_table[1][address & 0x03FF] = data;
        }
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) { // Palette region
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        ppu->palette_table[address] = data;
    }
}
