// PPU.c
#include "PPU.h"
#include "Cartridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>


static const uint32_t NES_PALETTE[64] = {
    // We have appended 0xFF as the LSB to signify a max 'alpha' value in RGBA8888 hex
    0x7C7C7CFF, // 0: Background Color (Black)
    0x0000FCFF, // 1
    0x0000BCFF, // 2
    0x4428BCFF, // 3
    0x940084FF, // 4
    0xA80020FF, // 5
    0xA81000FF, // 6
    0x881400FF, // 7
    0x503000FF, // 8
    0x007800FF, // 9
    0x006800FF, // 10
    0x005800FF, // 11
    0x004058FF, // 12
    0x000000FF, // 13
    0x000000FF, // 14
    0x000000FF, // 15
    0xBCBCBCFF, // 16
    0x0078F8FF, // 17
    0x0058F8FF, // 18
    0x6844FCFF, // 19
    0xD800CCFF, // 20
    0xE40058FF, // 21
    0xF83800FF, // 22
    0xE45C10FF, // 23
    0xAC7C00FF, // 24
    0x00B800FF, // 25
    0x00A800FF, // 26
    0x00A844FF, // 27
    0x008888FF, // 28
    0x000000FF, // 29
    0x000000FF, // 30
    0x000000FF, // 31
    0xF8F8F8FF, // 32
    0x3CBCFCFF, // 33
    0x6888FCFF, // 34
    0x9878F8FF, // 35
    0xF878F8FF, // 36
    0xF85898FF, // 37
    0xF87858FF, // 38
    0xFCA044FF, // 39
    0xF8B800FF, // 40
    0xB8F818FF, // 41
    0x58D854FF, // 42
    0x58F898FF, // 43
    0x00E8D8FF, // 44
    0x787878FF, // 45
    0x000000FF, // 46
    0x000000FF, // 47
    0xFCFCFCFF, // 48
    0xA4E4FCFF, // 49
    0xB8B8F8FF, // 50
    0xD8B8F8FF, // 51
    0xF8B8F8FF, // 52
    0xF8A4C0FF, // 53
    0xF0D0B0FF, // 54
    0xFCE0A8FF, // 55
    0xF8D878FF, // 56
    0xD8F878FF, // 57
    0xB8F8B8FF, // 58
    0xB8F8D8FF, // 59
    0x00FCFCFF, // 60
    0xF8D8F8FF, // 61
    0x000000FF, // 62
    0x000000FF  // 63
};

static inline uint32_t get_palette_colour(uint8_t i) {
    // Mask the index to ensure an index of 0-63
    // Convert colour in NES palette to colour that SDL can understand
    // I believe SDL understand colours as a 32 bit integer eg. 0xFF FF FF FF.
    uint8_t index = i & 0x3F;
    return NES_PALETTE[index];
}

Ppu* init_ppu() {
    Ppu* ppu = (Ppu*)malloc(sizeof(Ppu));

    ppu->cart = NULL;
    memset(&ppu->framebuffer, 0x00000000, PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT);
    memset(&ppu->name_table, 0, sizeof(ppu->name_table));
    memset(&ppu->palette_table, 0, sizeof(ppu->palette_table));
    memset(&ppu->pattern_table, 0, sizeof(ppu->pattern_table));

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
    memset(&ppu->framebuffer, 0x00000000, PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT);
    memset(&ppu->name_table, 0, sizeof(ppu->name_table));
    memset(&ppu->palette_table, 0, sizeof(ppu->palette_table));
    memset(&ppu->pattern_table, 0, sizeof(ppu->pattern_table));

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

void ppu_connect_cart(Ppu* ppu, Cartridge* cart) {
    ppu->cart = cart;
}

static inline uint8_t flipbyte(uint8_t b)
{
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
    return b;
}

void ppu_increment_scroll_x(Ppu* ppu) {
    if (ppu->registers.mask.bits.render_background || ppu->registers.mask.bits.render_sprites) {
        if (ppu->vram_addr.bits.coarse_x == 31) {
            ppu->vram_addr.bits.coarse_x = 0;
            ppu->vram_addr.bits.nametable_x = ~ppu->vram_addr.bits.nametable_x;
        }
        else {
            ppu->vram_addr.bits.coarse_x++;
        }
    }
}

void ppu_increment_scroll_y(Ppu* ppu) {
    if (ppu->registers.mask.bits.render_background || ppu->registers.mask.bits.render_sprites) {
        if (ppu->vram_addr.bits.fine_y < 7) {
            ppu->vram_addr.bits.fine_y++;
        }
        else {
            ppu->vram_addr.bits.fine_y = 0;
            if (ppu->vram_addr.bits.coarse_y == 29) {
                ppu->vram_addr.bits.coarse_y = 0;
                ppu->vram_addr.bits.nametable_y = ~ppu->vram_addr.bits.nametable_y;
            }
            else if (ppu->vram_addr.bits.coarse_y == 31) {
                ppu->vram_addr.bits.coarse_y = 0;
            }
            else {
                ppu->vram_addr.bits.coarse_y++;
            }
        }
    }
}

void ppu_transfer_address_x(Ppu* ppu) {
    if (ppu->registers.mask.bits.render_background || ppu->registers.mask.bits.render_sprites) {
        ppu->vram_addr.bits.nametable_x = ppu->tram_addr.bits.nametable_x;
        ppu->vram_addr.bits.coarse_x = ppu->tram_addr.bits.coarse_x;
    }
}

void ppu_transfer_address_y(Ppu* ppu) {
    if (ppu->registers.mask.bits.render_background || ppu->registers.mask.bits.render_sprites) {
        ppu->vram_addr.bits.fine_y = ppu->tram_addr.bits.fine_y;
        ppu->vram_addr.bits.nametable_y = ppu->tram_addr.bits.nametable_y;
        ppu->vram_addr.bits.coarse_y = ppu->tram_addr.bits.coarse_y;
    }
}

void ppu_load_bg_shifters(Ppu* ppu) {
    ppu->bg_shifter_pattern_lo = (ppu->bg_shifter_pattern_lo & 0xFF00) | ppu->bg_next_tile_lsb;
    ppu->bg_shifter_pattern_hi = (ppu->bg_shifter_pattern_hi & 0xFF00) | ppu->bg_next_tile_msb;
    ppu->bg_shifter_attrib_lo = (ppu->bg_shifter_attrib_lo & 0xFF00) | ((ppu->bg_next_tile_attr & 0b01) ? 0xFF : 0x00);
    ppu->bg_shifter_attrib_hi = (ppu->bg_shifter_attrib_hi & 0xFF00) | ((ppu->bg_next_tile_attr & 0b10) ? 0xFF : 0x00);
}

void ppu_update_shifters(Ppu* ppu) {
    if (ppu->registers.mask.bits.render_background) {
        ppu->bg_shifter_pattern_lo <<= 1;
        ppu->bg_shifter_pattern_hi <<= 1;
        ppu->bg_shifter_attrib_lo <<= 1;
        ppu->bg_shifter_attrib_hi <<= 1;
    }

    if (ppu->registers.mask.bits.render_sprites && ppu->cycle >= 1 && ppu->cycle < 258) {
        for (size_t i = 0; i < ppu->sprite_count; i++)
        {
            if (ppu->sprite_scanline[i].x > 0) {
                ppu->sprite_scanline[i].x--;
            } else {
                ppu->sprite_shifter_pattern_lo[i] <<= 1;
                ppu->sprite_shifter_pattern_hi[i] <<= 1;
            }
        }
    }
}

void ppu_clock(Ppu* ppu) {
    if (ppu->scanline >= -1 && ppu->scanline < 240) {
        if (ppu->scanline == 0 && ppu->cycle == 0 
            && (ppu->frames_completed % 2 != 0) 
            && (ppu->registers.mask.bits.render_background || ppu->registers.mask.bits.render_sprites)) {
            ppu->cycle = 1;
        }
        if (ppu->scanline == -1 && ppu->cycle == 1) {
            ppu->registers.status.bits.vertical_blank = 0;
            ppu->registers.status.bits.sprite_overflow = 0;
            ppu->registers.status.bits.sprite_zero_hit = 0;
            for (size_t i = 0; i < 8; i++) {
                ppu->sprite_shifter_pattern_lo[i] = 0;
                ppu->sprite_shifter_pattern_hi[i] = 0;
            }
        }
        if ((ppu->cycle >= 2 && ppu->cycle < 258) || (ppu->cycle >= 321 && ppu->cycle < 338)) {
            ppu_update_shifters(ppu);

            switch ((ppu->cycle - 1) % 8) {
                case 0:
                    ppu_load_bg_shifters(ppu);
                    ppu->bg_next_tile_id = ppu_read(ppu, 0x2000 | (ppu->vram_addr.reg & 0x0FFF));
                    break;
                case 2:
                    ppu->bg_next_tile_attr = ppu_read(ppu, 
                        0x23C0 
                        | (ppu->vram_addr.bits.nametable_y << 11)
                        | (ppu->vram_addr.bits.nametable_x << 10)
                        | ((ppu->vram_addr.bits.coarse_y >> 2) << 3)
                        | (ppu->vram_addr.bits.coarse_x >> 2)); 
                    if (ppu->vram_addr.bits.coarse_y & 0x02) ppu->bg_next_tile_attr >>= 4;
                    if (ppu->vram_addr.bits.coarse_x & 0x02) ppu->bg_next_tile_attr >>= 2;
                    ppu->bg_next_tile_attr &= 0x03;
                    break;
                case 4:
                    ppu->bg_next_tile_lsb = ppu_read(ppu, 
                        (ppu->registers.ctrl.bits.pattern_background << 12)
                        + ((uint16_t)ppu->bg_next_tile_id << 4)
                        + (ppu->vram_addr.bits.fine_y) + 0);
                    break;
                case 6:
                    ppu->bg_next_tile_msb = ppu_read(ppu, 
                        (ppu->registers.ctrl.bits.pattern_background << 12)
                        + ((uint16_t)ppu->bg_next_tile_id << 4)
                        + (ppu->vram_addr.bits.fine_y) + 8);
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
            ppu->bg_next_tile_id = ppu_read(ppu, (0x2000 | (ppu->vram_addr.reg & 0x0FFF)));
        }
        if (ppu->scanline == -1 && ppu->cycle >= 280 && ppu->cycle < 305) {
            ppu_transfer_address_y(ppu);
        }
    }

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
            
            if (diff >= 0 && diff < (ppu->registers.ctrl.bits.sprite_size ? 16 : 8) 
                && ppu->sprite_count < 8) {
                if (ppu->sprite_count < 8) {
                    if (n_oam_entry == 0) {
                        ppu->b_sprite_zero_hit_possible = true;
                    }
                    memcpy(&ppu->sprite_scanline[ppu->sprite_count], 
                           &ppu->oam[n_oam_entry], 
                           sizeof(sObjectAttributeEntry));
                }
                ppu->sprite_count++;
            }
            n_oam_entry++;
        }
        // Set sprite overflow flag
        ppu->registers.status.bits.sprite_overflow = (ppu->sprite_count >= 8);
    }

    if (ppu->cycle == 340) {
        for (uint8_t i = 0; i < ppu->sprite_count; i++) {
            uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
            uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;

            if (!ppu->registers.ctrl.bits.sprite_size) {
                // 8x8 Sprite
                if (!(ppu->sprite_scanline[i].attribute & 0x80)) {
                    // Sprite normal
                    sprite_pattern_addr_lo = 
                        (ppu->registers.ctrl.bits.pattern_sprite << 12)
                      | (ppu->sprite_scanline[i].id << 4)
                      | (ppu->scanline - ppu->sprite_scanline[i].y);
                } else {
                    // Sprite flipped vertically
                    if (ppu->scanline - ppu->sprite_scanline[i].y < 8) {
                        sprite_pattern_addr_lo = 
                            (ppu->registers.ctrl.bits.pattern_sprite << 12)
                          | (ppu->sprite_scanline[i].id << 4)
                          | (7 - (ppu->scanline - ppu->sprite_scanline[i].y));
                    } else {
                        sprite_pattern_addr_lo = 
                            (ppu->registers.ctrl.bits.pattern_sprite << 12)
                          | (ppu->sprite_scanline[i].id << 4)
                          | (7 - (ppu->scanline - ppu->sprite_scanline[i].y));
                    }
                }
            } else {
                // 8x16 Sprite Mode
                if (!(ppu->sprite_scanline[i].attribute & 0x80)) {
                    // Sprite NOT flipped vertically
                    if (ppu->scanline - ppu->sprite_scanline[i].y < 8) {
                        // Top half
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12)
                          | ((ppu->sprite_scanline[i].id & 0xFE) << 4)
                          | ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
                    } else {
                        // Bottom half
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12)
                          | (((ppu->sprite_scanline[i].id & 0xFE) + 1) << 4)
                          | ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07);
                    }
                } else {
                    // Sprite flipped vertically
                    if (ppu->scanline - ppu->sprite_scanline[i].y < 8) {
                        // Reading top half tile
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12)
                          | (((ppu->sprite_scanline[i].id & 0xFE) + 1) << 4)
                          | (7 - ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07));
                    } else {
                        // Bottom half tile
                        sprite_pattern_addr_lo = 
                            ((ppu->sprite_scanline[i].id & 0x01) << 12)
                          | ((ppu->sprite_scanline[i].id & 0xFE) << 4)
                          | (7 - ((ppu->scanline - ppu->sprite_scanline[i].y) & 0x07));
                    }
                }
            }

            // Hi bit plane always offset by 8 bytes
            sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;

            // Fetch pattern data
            sprite_pattern_bits_lo = ppu_read(ppu, sprite_pattern_addr_lo);
            sprite_pattern_bits_hi = ppu_read(ppu, sprite_pattern_addr_hi);

            // Flip horizontally if needed
            if (ppu->sprite_scanline[i].attribute & 0x40) {
                sprite_pattern_bits_lo = flipbyte(sprite_pattern_bits_lo);
                sprite_pattern_bits_hi = flipbyte(sprite_pattern_bits_hi);
            }

            // Load into shifters
            ppu->sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
            ppu->sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
        }
    }

    if (ppu->scanline == 240) {
        // Post-render scanline; nothing happening here
    }

    if (ppu->scanline >= 241 && ppu->scanline < 261) {
        if (ppu->scanline == 241 && ppu->cycle == 1) {
            ppu->registers.status.bits.vertical_blank = 1;
            if (ppu->registers.ctrl.bits.enable_nmi) {
                ppu->nmi_occurred = true;
            }
        }
    }

    uint8_t bg_pixel = 0x00;
    uint8_t bg_palette = 0x00;

    if (ppu->registers.mask.bits.render_background) {
        if (ppu->registers.mask.bits.render_background_left || (ppu->cycle >= 9)) {
            uint16_t bit_mux = 0x8000 >> ppu->fine_x;
            uint8_t p0_pixel = (ppu->bg_shifter_pattern_lo & bit_mux) > 0;
            uint8_t p1_pixel = (ppu->bg_shifter_pattern_hi & bit_mux) > 0;
            bg_pixel = (p1_pixel << 1) | p0_pixel;

            uint8_t bg_palette0 = (ppu->bg_shifter_attrib_lo & bit_mux) > 0;
            uint8_t bg_palette1 = (ppu->bg_shifter_attrib_hi & bit_mux) > 0;
            bg_palette = (bg_palette1 << 1) | bg_palette0;
        }
    }

    uint8_t fg_pixel = 0x00;
    uint8_t fg_palette = 0x00; 
    uint8_t fg_priority = 0x00;

    if (ppu->registers.mask.bits.render_sprites) {
        if (ppu->registers.mask.bits.render_sprites_left || (ppu->cycle >= 9)) {
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

    uint8_t pixel = 0x00;   
    uint8_t palette = 0x00; 

    if (bg_pixel == 0 && fg_pixel == 0) {
        // Both transparent
        pixel = 0x00;
        palette = 0x00;
    } else if (bg_pixel == 0 && fg_pixel > 0) {
        // Background transparent, sprite visible
        pixel = fg_pixel;
        palette = fg_palette;
    } else if (bg_pixel > 0 && fg_pixel == 0) {
        // Background visible, sprite transparent
        pixel = bg_pixel;
        palette = bg_palette;
    } else if (bg_pixel > 0 && fg_pixel > 0) {
        // Both visible
        if (fg_priority) {
            pixel = fg_pixel;
            palette = fg_palette;
        } else {
            pixel = bg_pixel;
            palette = bg_palette;
        }

        // Sprite zero hit check
        if (ppu->b_sprite_zero_hit_possible && ppu->b_sprite_zero_being_rendered) {
            if (ppu->registers.mask.bits.render_background 
                & ppu->registers.mask.bits.render_sprites) {
                if (!(ppu->registers.mask.bits.render_background_left 
                      | ppu->registers.mask.bits.render_sprites_left)) {
                    if (ppu->cycle >= 9 && ppu->cycle < 258) {
                        ppu->registers.status.bits.sprite_zero_hit = 1;
                    }
                } else {
                    if (ppu->cycle >= 1 && ppu->cycle < 258) {
                        ppu->registers.status.bits.sprite_zero_hit = 1;
                    }
                }
            }
        }
    }

    // Update framebuffer
    if ((ppu->cycle - 1) >= 0 && (ppu->cycle - 1) < PPU_SCREEN_WIDTH 
        && ppu->scanline >= 0 && ppu->scanline < PPU_SCREEN_HEIGHT) {
        ppu->framebuffer[ppu->scanline * PPU_SCREEN_WIDTH + (ppu->cycle - 1)] =
            get_palette_colour(ppu_read(ppu, 0x3F00 + (palette << 2) + pixel));
    }

    // Advance renderer
    ppu->cycle++;
    if (ppu->registers.mask.bits.render_background || ppu->registers.mask.bits.render_sprites) {
        if (ppu->cycle == 260 && ppu->scanline < 240) {
            // Typically where Mappers 4/5 intercept to handle special scanline IRQ
            // Not yet implemented
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


// Ability for CPU to read/write PPU registers
uint8_t cpu_ppu_read(Ppu* ppu, uint16_t address, bool rd_only) {
    uint8_t data = 0x00;
    if (rd_only) {
        switch (address) {
            case 0x0000: // Control
                data = ppu->registers.ctrl.reg;
                break;
            case 0x0001: // Mask
                data = ppu->registers.mask.reg;
                break;
            case 0x0002: // Status
                data = ppu->registers.status.reg;
                break;
            case 0x0003: // OAM Address
                break;
            case 0x0004: // OAM Data
                break;
            case 0x0005: // Scroll
                break;
            case 0x0006: // PPU Address
                break;
            case 0x0007: // PPU Data
                break;
        }
    }
    else {
        switch (address) {
            // Control - Not readable
            case 0x0000: break;
            
            // Mask - Not Readable
            case 0x0001: break;
            
            // Status
            case 0x0002:
                data = (ppu->registers.status.reg & 0xE0) | (ppu->ppu_data_buffer & 0x1F);
                ppu->registers.status.bits.vertical_blank = 0;
                ppu->address_latch = 0;
                break;

            // OAM Address
            case 0x0003: break;

            // OAM Data
            case 0x0004:
                data = ppu->p_oam[ppu->oam_addr];
                break;

            // Scroll - Not Readable
            case 0x0005: break;

            // PPU Address - Not Readable
            case 0x0006: break;

            // PPU Data
            case 0x0007: 
                data = ppu->ppu_data_buffer;
                ppu->ppu_data_buffer = ppu_read(ppu, ppu->vram_addr.reg);
                if (ppu->vram_addr.reg >= 0x3F00) {
                    data = ppu->ppu_data_buffer;
                }
                ppu->vram_addr.reg += ppu->registers.ctrl.bits.increment_mode ? 32 : 1;
                break;
        }
    }
    return data;
}

void cpu_ppu_write(Ppu* ppu, uint16_t address, uint8_t data) {
    switch (address) {
        case 0x0000: // Control
            ppu->registers.ctrl.reg = data;
            ppu->tram_addr.bits.nametable_x = ppu->registers.ctrl.bits.nametable_x;
            ppu->tram_addr.bits.nametable_y = ppu->registers.ctrl.bits.nametable_y;
            break;
        case 0x0001: // Mask
            ppu->registers.mask.reg = data;
            break;
        case 0x0002: // Status
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
                ppu->tram_addr.bits.coarse_x = data >> 3;
                ppu->address_latch = 1;
            }
            else {
                ppu->tram_addr.bits.fine_y = data & 0x07;
                ppu->tram_addr.bits.coarse_y = data >> 3;
                ppu->address_latch = 0;
            }
            break;
        case 0x0006: // PPU Address
            if (ppu->address_latch == 0) {
                ppu->tram_addr.reg = (uint16_t)((data & 0x3F) << 8) | (ppu->tram_addr.reg & 0x00FF);
                ppu->address_latch = 1;
            }
            else {
                ppu->tram_addr.reg = (ppu->tram_addr.reg & 0xFF00) | data;
                ppu->vram_addr = ppu->tram_addr; 
                ppu->address_latch = 0;
            }
            break;
        case 0x0007: // PPU Data
            ppu_write(ppu, ppu->vram_addr.reg, data);
            ppu->vram_addr.reg += ppu->registers.ctrl.bits.increment_mode ? 32 : 1;
            break;
    }
}


// Ability for PPU to read and write data
uint8_t ppu_read(Ppu* ppu, uint16_t address) {
    uint8_t data = 0x00;
    address &= 0x3FFF;

    if (cartridge_ppu_read(ppu->cart, address, &data)) {
        // Cartridge handled read
    }
    // Pattern table
    else if (address >= 0x0000 && address <= 0x1FFF) {
        // Get the most significant bit of the address and offsets by the rest of the bits of the address
        data = ppu->pattern_table[(address & 0x1000) >> 12][address & 0x0FFF];
    }
    // Name table
    else if (address >= 0x2000 && address <= 0x3EFF) {
        address &= 0x0FFF;
        if (ppu->cart->mirror == VERTICAL) {
            if (address >= 0x0000 && address <= 0x03FF) data = ppu->name_table[0][address & 0x03FF];
            if (address >= 0x0400 && address <= 0x07FF) data = ppu->name_table[1][address & 0x03FF];
            if (address >= 0x0800 && address <= 0x0BFF) data = ppu->name_table[0][address & 0x03FF];
            if (address >= 0x0C00 && address <= 0x0FFF) data = ppu->name_table[1][address & 0x03FF];
        }
        else if (ppu->cart->mirror == HORIZONTAL) {
            if (address >= 0x0000 && address <= 0x03FF) data = ppu->name_table[0][address & 0x03FF];
            if (address >= 0x0400 && address <= 0x07FF) data = ppu->name_table[0][address & 0x03FF];
            if (address >= 0x0800 && address <= 0x0BFF) data = ppu->name_table[1][address & 0x03FF];
            if (address >= 0x0C00 && address <= 0x0FFF) data = ppu->name_table[1][address & 0x03FF];
        }
    }
    // Palette
    else if (address >= 0x3F00 && address <= 0x3FFF) {
        address &= 0x001F; // Mask less significant 5 bits

        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        data = ppu->palette_table[address];
    }

    return data;
}

void ppu_write(Ppu* ppu, uint16_t address, uint8_t data) {
    address &= 0x3FFF;

    if (cartridge_ppu_write(ppu->cart, address, data)) {
        // Cartridge handled write
    }
    // Pattern table
    else if (address >= 0x0000 && address <= 0x1FFF) {
        // This memory acts as a ROM for the PPU,
        // but for some NES ROMs, this memory is a RAM.
        ppu->pattern_table[(address & 0x1000) >> 12][address & 0x0FFF] = data;
    }
    // Name table
    else if (address >= 0x2000 && address <= 0x3EFF) {
        address &= 0x0FFF;
        if (ppu->cart->mirror == VERTICAL) {
            if (address >= 0x0000 && address <= 0x03FF) ppu->name_table[0][address & 0x03FF] = data;
            if (address >= 0x0400 && address <= 0x07FF) ppu->name_table[1][address & 0x03FF] = data;
            if (address >= 0x0800 && address <= 0x0BFF) ppu->name_table[0][address & 0x03FF] = data;
            if (address >= 0x0C00 && address <= 0x0FFF) ppu->name_table[1][address & 0x03FF] = data;
        }
        else if (ppu->cart->mirror == HORIZONTAL) {
            if (address >= 0x0000 && address <= 0x03FF) ppu->name_table[0][address & 0x03FF] = data;
            if (address >= 0x0400 && address <= 0x07FF) ppu->name_table[0][address & 0x03FF] = data;
            if (address >= 0x0800 && address <= 0x0BFF) ppu->name_table[1][address & 0x03FF] = data;
            if (address >= 0x0C00 && address <= 0x0FFF) ppu->name_table[1][address & 0x03FF] = data;
        }
    }
    // Palette
    else if (address >= 0x3F00 && address <= 0x3FFF) {
        address &= 0x001F; // Mask less significant 5 bits
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        ppu->palette_table[address] = data;
    }
}
