// PPU.c
#include "PPU.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Bus Bus;

// Forward declarations of internal functions
static void ppu_increment_x(Ppu* ppu);
static void ppu_increment_y(Ppu* ppu);
static void ppu_fetch_background_data(Ppu* ppu);
static void ppu_shift_registers(Ppu* ppu);
static void ppu_update_shifters(Ppu* ppu);
static void ppu_evaluate_sprites(Ppu* ppu);
static uint8_t ppu_background_pixel(Ppu* ppu);
static uint8_t ppu_sprite_pixel(Ppu* ppu, uint8_t* sprite_index);
static void ppu_render_pixel(Ppu* ppu);
static void ppu_set_vertical_blank(Ppu* ppu);
static void ppu_clear_vertical_blank(Ppu* ppu);
static uint8_t ppu_read_palette(Ppu* ppu, uint16_t addr);
static void ppu_increment_v(Ppu* ppu);

// Helper macro for reversing bits (used for horizontal flip)
#define RBIT(x) (((x * 0x0802U & 0x22110U) | (x * 0x8020U & 0x88440U)) * 0x10101U >> 16)

static const uint32_t NES_PALETTE[64] = {
    0x7C7C7C, // 0: Background Color (Black)
    0x0000FC, // 1
    0x0000BC, // 2
    0x4428BC, // 3
    0x940084, // 4
    0xA80020, // 5
    0xA81000, // 6
    0x881400, // 7
    0x503000, // 8
    0x007800, // 9
    0x006800, // 10
    0x005800, // 11
    0x004058, // 12
    0x000000, // 13
    0x000000, // 14
    0x000000, // 15
    0xBCBCBC, // 16
    0x0078F8, // 17
    0x0058F8, // 18
    0x6844FC, // 19
    0xD800CC, // 20
    0xE40058, // 21
    0xF83800, // 22
    0xE45C10, // 23
    0xAC7C00, // 24
    0x00B800, // 25
    0x00A800, // 26
    0x00A844, // 27
    0x008888, // 28
    0x000000, // 29
    0x000000, // 30
    0x000000, // 31
    0xF8F8F8, // 32
    0x3CBCFC, // 33
    0x6888FC, // 34
    0x9878F8, // 35
    0xF878F8, // 36
    0xF85898, // 37
    0xF87858, // 38
    0xFCA044, // 39
    0xF8B800, // 40
    0xB8F818, // 41
    0x58D854, // 42
    0x58F898, // 43
    0x00E8D8, // 44
    0x787878, // 45
    0x000000, // 46
    0x000000, // 47
    0xFCFCFC, // 48
    0xA4E4FC, // 49
    0xB8B8F8, // 50
    0xD8B8F8, // 51
    0xF8B8F8, // 52
    0xF8A4C0, // 53
    0xF0D0B0, // 54
    0xFCE0A8, // 55
    0xF8D878, // 56
    0xD8F878, // 57
    0xB8F8B8, // 58
    0xB8F8D8, // 59
    0x00FCFC, // 60
    0xF8D8F8, // 61
    0x000000, // 62
    0x000000  // 63
};

// Update the make_colour function to use the NES palette
static inline uint32_t make_colour(uint8_t i) {
    // Mask the index to ensure an index of 0-63
    // Convert colour in NES palette to colour that SDL can understand
        // I believe SDL understand colours as a 32 bit integer eg. 0x FF FF FF FF.
    uint8_t index = i & 0x3F;
    return 0xFF000000 | NES_PALETTE[index];
}

// Initialize the PPU
Ppu* init_ppu() {
    Ppu* ppu = (Ppu*)malloc(sizeof(Ppu));
    if (!ppu) {
        fprintf(stderr, "[PPU] Memory allocation failed\n");
        exit(1);
    }
    memset(ppu, 0, sizeof(*ppu));
    ppu->reg.PPUSTATUS = 0xA0; // As on power-up
    printf("[PPU] Memory allocated.\n");

    // Initialize VRAM, Nametables, and Palettes
    memset(ppu->pattern_tables, 0, sizeof(ppu->pattern_tables));
    printf("[PPU] Allocated pattern table.\n");
    memset(ppu->nametables, 0, sizeof(ppu->nametables));
    printf("[PPU] Allocated nametables.\n");
    memset(ppu->palettes, 0, sizeof(ppu->palettes));
    printf("[PPU] Allocated pallete table.\n");

    return ppu;
}

void connect_ppu_cart(Ppu* ppu, Cartridge* cart) {
    ppu->cart = cart;
    // Initialize CHR Data
    if (ppu->cart->nCHRBanks > 0) {
        ppu->chr_data = ppu->cart->CHRMemory;
        ppu->chr_size = sizeof(ppu->cart->CHRMemory);
        ppu->chr_ram = false;
        printf("PPU: Using CHR ROM (%zu KB)\n", ppu->chr_size / 1024);
    } else {
        // Allocate CHR RAM (8KB is typical)
        ppu->chr_size = 8 * 1024;
        ppu->chr_data = malloc(ppu->chr_size);
        if (!ppu->chr_data) {
            fprintf(stderr, "PPU: Failed to allocate CHR RAM\n");
            free(ppu);
            exit(1);
        }
        memset(ppu->chr_data, 0, ppu->chr_size);
        ppu->chr_ram = true;
        printf("PPU: Using CHR RAM (%zu KB)\n", ppu->chr_size / 1024);
    }
}

// Check if a frame is done
bool ppu_is_frame_done(Ppu* ppu) {
    return ppu->frame_done;
}

// Clear the frame done flag
void ppu_clear_frame_done(Ppu* ppu) {
    ppu->frame_done = false;
}

// Check if NMI occurred
bool ppu_nmi_occurred(Ppu* ppu) {
    return ppu->nmi_occurred;
}

// Clear the NMI flag
void ppu_clear_nmi(Ppu* ppu) {
    ppu->nmi_occurred = false;
}

// Read a PPU register
uint8_t ppu_read(Ppu* ppu, uint16_t address) {
    address &= 0x2007; 
    switch (address) {
        case 0x2002: { // PPUSTATUS
            uint8_t data = ppu->reg.PPUSTATUS;
            // Clear vblank
            ppu->reg.PPUSTATUS &= ~0x80;
            ppu->w = false;
            return data;
        }
        case 0x2004: { // OAMDATA
            return ppu->OAM[ppu->reg.OAMADDR];
        }
        case 0x2007: { // PPUDATA
            uint16_t addr = ppu->v & 0x3FFF;
            uint8_t data = 0;
            if (addr < 0x2000) { // Pattern Tables
                data = ppu->pattern_tables[addr];
            } else if (addr < 0x3000) { // Nametables
                data = ppu->nametables[addr - 0x2000];
            } else { // Palettes
                data = ppu->palettes[addr - 0x3F00];
            }

            // The PPU has a buffered read
            uint8_t buffered = ppu->reg.PPUDATA;
            ppu->reg.PPUDATA = data;
            if (addr >= 0x3F00) {
                // Palette reads return immediate data
                return data;
            } else {
                // Return buffered data
                ppu_increment_v(ppu);
                return buffered;
            }
        }
        default:
            return 0;
    }
}

// Write a PPU register from CPU
void ppu_write(Ppu* ppu, uint16_t address, uint8_t value) {
    address &= 0x2007;
    switch (address) {
        case 0x2000: // PPUCTRL
            ppu->reg.PPUCTRL = value;
            ppu->t = (ppu->t & 0xF3FF) | ((value & 0x03) << 10);
            break;
        case 0x2001: // PPUMASK
            ppu->reg.PPUMASK = value;
            break;
        case 0x2003: // OAMADDR
            ppu->reg.OAMADDR = value;
            break;
        case 0x2004: // OAMDATA
            ppu->OAM[ppu->reg.OAMADDR++] = value;
            break;
        case 0x2005: // PPUSCROLL
            if (!ppu->w) {
                // First write
                ppu->x = value & 0x07;
                ppu->t = (ppu->t & 0xFFE0) | ((value & 0xF8) >> 3);
                ppu->w = true;
            } else {
                // Second write
                ppu->t = (ppu->t & 0x8C1F) | ((value & 0xF8) << 2);
                ppu->t = (ppu->t & 0xFC1F) | ((value & 0x07) << 12);
                ppu->w = false;
            }
            break;
        case 0x2006: // PPUADDR
            if (!ppu->w) {
                ppu->t = (ppu->t & 0x00FF) | ((value & 0x3F) << 8);
                ppu->w = true;
            } else {
                ppu->t = (ppu->t & 0x7F00) | value;
                ppu->v = ppu->t;
                ppu->w = false;
            }
            break;
        case 0x2007: { // PPUDATA
            uint16_t addr = ppu->v & 0x3FFF;
            if (addr < 0x2000) { // Pattern Tables
                ppu->pattern_tables[addr] = value;
            } else if (addr < 0x3000) { // Nametables
                ppu->nametables[addr - 0x2000] = value;
            } else { // Palettes
                uint16_t palette_addr = addr - 0x3F00;
                if (palette_addr % 4 == 0 && palette_addr > 0) {
                    // Mirror of 0x3F00
                    ppu->palettes[0] = value;
                }
                ppu->palettes[palette_addr & 0x1F] = value;
            }
            ppu_increment_v(ppu);
            break;
        }
        default:
            break;
    }
}

// Internal increment v based on PPUCTRL increment mode
static void ppu_increment_v(Ppu* ppu) {
    if (ppu->reg.PPUCTRL & 0x04) {
        ppu->v += 32;
    } else {
        ppu->v += 1;
    }
}

// Rendering steps

// Increment coarse X
static void ppu_increment_x(Ppu* ppu) {
    if ((ppu->v & 0x001F) == 31) {
        ppu->v &= ~0x001F;
        ppu->v ^= 0x0400;
    } else {
        ppu->v++;
    }
}

// Increment Y
static void ppu_increment_y(Ppu* ppu) {
    if ((ppu->v & 0x7000) != 0x7000) {
        ppu->v += 0x1000;
    } else {
        ppu->v &= ~0x7000;
        uint16_t y = (ppu->v & 0x03E0) >> 5;
        if (y == 29) {
            y = 0;
            ppu->v ^= 0x0800;
        } else if (y == 31) {
            y = 0;
        } else {
            y++;
        }
        ppu->v = (ppu->v & ~0x03E0) | (y << 5);
    }
}

// Transfer x from t to v at start of horizontal blank of pre-render scanline
static void ppu_transfer_x(Ppu* ppu) {
    ppu->v = (ppu->v & 0xFBE0) | (ppu->t & 0x041F);
}

// Transfer y from t to v at start of vertical blank of pre-render scanline
static void ppu_transfer_y(Ppu* ppu) {
    ppu->v = (ppu->v & 0x841F) | (ppu->t & 0x7BE0);
}

// Read from palette memory
static uint8_t ppu_read_palette(Ppu* ppu, uint16_t addr) {
    addr &= 0x1F;
    // Palette is at 3F00-3F1F
    return ppu->palettes[addr];
}

// Fetch background data
static void ppu_fetch_background_data(Ppu* ppu) {
    uint16_t base_nametable = 0x2000;
    uint16_t addr = base_nametable | (ppu->v & 0x0FFF);
    ppu->nametable_byte = ppu->nametables[addr - 0x2000];

    uint16_t attr_addr = 0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07);
    ppu->attribute_byte = ppu->nametables[attr_addr - 0x2000];
    // Shift attribute bits
    uint8_t shift = ((ppu->v >> 4) & 4) | (ppu->v & 2);
    ppu->attribute_byte = ((ppu->attribute_byte >> shift) & 0x03);

    uint16_t pattern_base = (ppu->reg.PPUCTRL & 0x10) ? 0x1000 : 0x0000;
    uint16_t pattern_addr = pattern_base + (ppu->nametable_byte * 16) + ((ppu->v >> 12) & 7);
    ppu->low_pattern_byte = ppu->pattern_tables[pattern_addr];
    ppu->high_pattern_byte = ppu->pattern_tables[pattern_addr + 8];
}

// Update shift registers before fetching next tile
static void ppu_update_shifters(Ppu* ppu) {
    ppu->pattern_shift_low = (ppu->pattern_shift_low & 0xFF00) | ppu->low_pattern_byte;
    ppu->pattern_shift_high = (ppu->pattern_shift_high & 0xFF00) | ppu->high_pattern_byte;

    uint8_t palette_low = (ppu->attribute_byte & 0x02) ? 0xFF : 0x00;
    uint8_t palette_high = (ppu->attribute_byte & 0x01) ? 0xFF : 0x00;

    ppu->attribute_shift_low = (ppu->attribute_shift_low & 0xFF00) | palette_low;
    ppu->attribute_shift_high = (ppu->attribute_shift_high & 0xFF00) | palette_high;
}

// Shift background shift registers
static void ppu_shift_registers(Ppu* ppu) {
    if (ppu->reg.PPUMASK & 0x08) {
        ppu->pattern_shift_low <<= 1;
        ppu->pattern_shift_high <<= 1;
        ppu->attribute_shift_low <<= 1;
        ppu->attribute_shift_high <<= 1;
    }
}

// Evaluate sprites on the current scanline
static void ppu_evaluate_sprites(Ppu* ppu) {
    // Clear secondary OAM
    memset(ppu->secondary_OAM, 0xFF, 32);
    uint8_t count = 0;
    uint8_t sprite_height = (ppu->reg.PPUCTRL & 0x20) ? 16 : 8;
    for (uint8_t i = 0; i < 64; i++) {
        uint8_t y = ppu->OAM[i*4+0];
        uint8_t x = ppu->OAM[i*4+3];
        if (ppu->scanline >= y && ppu->scanline < (uint16_t)(y + sprite_height)) {
            if (count < 8) {
                uint8_t tile = ppu->OAM[i*4+1];
                uint8_t attr = ppu->OAM[i*4+2];
                ppu->secondary_OAM[count*4+0] = y;
                ppu->secondary_OAM[count*4+1] = tile;
                ppu->secondary_OAM[count*4+2] = attr;
                ppu->secondary_OAM[count*4+3] = x;
                if (i == 0) ppu->sprite_zero_hit_possible = true;
                count++;
            } else {
                ppu->reg.PPUSTATUS |= 0x20; // sprite overflow
                break;
            }
        }
    }
    ppu->sprite_count = count;

    // Fetch sprite patterns
    for (uint8_t i = 0; i < count; i++) {
        uint8_t y = ppu->secondary_OAM[i*4+0];
        uint8_t tile = ppu->secondary_OAM[i*4+1];
        uint8_t attr = ppu->secondary_OAM[i*4+2];
        uint8_t x = ppu->secondary_OAM[i*4+3];

        uint8_t sprite_height = (ppu->reg.PPUCTRL & 0x20) ? 16 : 8;
        int line = ppu->scanline - y;
        if (attr & 0x80) {
            // vertical flip
            line = sprite_height - 1 - line;
        }
        uint16_t pattern_addr;
        if (!(ppu->reg.PPUCTRL & 0x20)) {
            // 8x8 sprite
            uint16_t base = (ppu->reg.PPUCTRL & 0x08) ? 0x1000 : 0x0000;
            pattern_addr = base + tile*16 + line;
        } else {
            // 8x16 sprite
            uint16_t base = (tile & 1) ? 0x1000 : 0x0000;
            tile &= 0xFE;
            pattern_addr = base + tile*16 + (line & 7) + ((line & 8) ? 8 : 0);
        }

        uint8_t low = ppu->pattern_tables[pattern_addr];
        uint8_t high = ppu->pattern_tables[pattern_addr + 8];

        if (attr & 0x40) {
            // horizontal flip
            low = (uint8_t)RBIT(low);
            high = (uint8_t)RBIT(high);
        }

        ppu->sprite_pattern_low[i] = low;
        ppu->sprite_pattern_high[i] = high;
        ppu->sprite_data[i].y = y;
        ppu->sprite_data[i].tile_index = tile;
        ppu->sprite_data[i].attributes = attr;
        ppu->sprite_data[i].x = x;
    }
}

// Get background pixel color
static uint8_t ppu_background_pixel(Ppu* ppu) {
    if (!(ppu->reg.PPUMASK & 0x08)) return 0; // Background not enabled
    uint16_t bit0 = (ppu->pattern_shift_low & 0x8000) >> 15;
    uint16_t bit1 = (ppu->pattern_shift_high & 0x8000) >> 14;
    uint8_t pixel = (bit1 << 1) | bit0;

    uint16_t att0 = (ppu->attribute_shift_low & 0x8000) >> 15;
    uint16_t att1 = (ppu->attribute_shift_high & 0x8000) >> 14;
    uint8_t palette = (att1 << 1) | att0;

    if (pixel == 0) return 0; // transparent
    return ppu_read_palette(ppu, (palette << 2) + pixel);
}

// Get sprite pixel color
static uint8_t ppu_sprite_pixel(Ppu* ppu, uint8_t* sprite_index) {
    if (!(ppu->reg.PPUMASK & 0x10)) return 0; // Sprites not enabled
    uint8_t x = (uint8_t)(ppu->cycle - 1);
    for (uint8_t i = 0; i < ppu->sprite_count; i++) {
        uint8_t offset = x - ppu->sprite_data[i].x;
        if (offset < 8) {
            uint8_t bit0 = (ppu->sprite_pattern_low[i] & (0x80 >> offset)) ? 1 : 0;
            uint8_t bit1 = (ppu->sprite_pattern_high[i] & (0x80 >> offset)) ? 1 : 0;
            uint8_t pixel = (bit1 << 1) | bit0;
            if (pixel == 0) continue; // transparent
            uint8_t palette = (ppu->sprite_data[i].attributes & 0x3) + 4;
            uint8_t c = ppu_read_palette(ppu, (palette << 2) + pixel);
            *sprite_index = i;
            return c;
        }
    }
    return 0;
}

// Render a single pixel
static void ppu_render_pixel(Ppu* ppu) {
    int x = ppu->cycle - 1;
    int y = ppu->scanline;
    if (x < 0 || x >= PPU_SCREEN_WIDTH || y < 0 || y >= PPU_SCREEN_HEIGHT)
        return;

    uint8_t bg = ppu_background_pixel(ppu);
    uint8_t sprite_index = 0xFF;
    uint8_t fg = ppu_sprite_pixel(ppu, &sprite_index);

    uint8_t final_color = 0;

    if (bg == 0 && fg == 0) {
        final_color = ppu_read_palette(ppu, 0);
    } else if (bg == 0 && fg != 0) {
        final_color = fg;
    } else if (bg != 0 && fg == 0) {
        final_color = bg;
    } else {
        // Both non-transparent, priority
        if (ppu->sprite_data[sprite_index].attributes & 0x20) {
            // Background in front
            final_color = bg;
        } else {
            // Sprite in front
            final_color = fg;
        }
        // Sprite zero hit
        if (sprite_index == 0 && ppu->sprite_zero_hit_possible && x < 255) {
            ppu->reg.PPUSTATUS |= 0x40;
        }
    }

    ppu->framebuffer[y * PPU_SCREEN_WIDTH + x] = make_colour(final_color);
}

// Set vertical blank flag and trigger NMI if enabled
static void ppu_set_vertical_blank(Ppu* ppu) {
    ppu->reg.PPUSTATUS |= 0x80;
    if (ppu->reg.PPUCTRL & 0x80) {
        ppu->nmi_occurred = true;
    }
}

// Clear vertical blank flag
static void ppu_clear_vertical_blank(Ppu* ppu) {
    ppu->reg.PPUSTATUS &= ~0x80;
}

// PPU clock - advance the PPU by one cycle
void ppu_clock(Ppu* ppu) {
    bool rendering_enabled = (ppu->reg.PPUMASK & 0x18) != 0;

    if (ppu->scanline < 240) {
        if (ppu->scanline >= 0 && ppu->scanline < 240) {
            if (ppu->cycle == 0) {
                // Do nothing
            }
            if ((ppu->cycle >= 1 && ppu->cycle <= 256) || (ppu->cycle >= 321 && ppu->cycle <= 336)) {
                ppu_shift_registers(ppu);
                switch ((ppu->cycle - 1) % 8) {
                    case 0:
                        ppu_update_shifters(ppu);
                        ppu_fetch_background_data(ppu);
                        break;
                    case 7:
                        ppu_increment_x(ppu);
                        break;
                }
            }
            if (ppu->cycle == 256) {
                ppu_increment_y(ppu);
            }
            if (ppu->cycle == 257) {
                ppu_update_shifters(ppu);
                ppu_transfer_x(ppu);
                if (ppu->scanline >= 0 && ppu->scanline < 240) {
                    ppu_evaluate_sprites(ppu);
                }
            }
            if (ppu->cycle == 337 || ppu->cycle == 339) {
                ppu_fetch_background_data(ppu);
            }

            if (ppu->cycle > 0 && ppu->cycle <= 256 && rendering_enabled) {
                ppu_render_pixel(ppu);
            }

        }
    }

    if (ppu->scanline == 241 && ppu->cycle == 1) {
        ppu_set_vertical_blank(ppu);
    }

    if (ppu->scanline == 261) {
        if (ppu->cycle == 1) {
            ppu_clear_vertical_blank(ppu);
            ppu->reg.PPUSTATUS &= ~0x40; // clear sprite zero hit
            ppu->reg.PPUSTATUS &= ~0x20; // clear sprite overflow
            ppu->sprite_zero_hit_possible = false;
        }
        if (ppu->cycle >= 280 && ppu->cycle <= 304 && rendering_enabled) {
            ppu_transfer_y(ppu);
        }
    }

    // Increment cycle
    ppu->cycle++;
    if (ppu->cycle > 340) {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline > 261) {
            ppu->scanline = 0;
            ppu->frame_done = true;
        }
    }

    // printf("PPU CYCLE %d\n", ppu->cycle);
    // printf("PPU SCANLINE %d\n", ppu->scanline);
    // printf(ppu->framebuffer);
}
