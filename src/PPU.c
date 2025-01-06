// PPU.c
#include "PPU.h"
#include "Cartridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
        // I believe SDL understand colours as a 32 bit integer eg. 0x FF FF FF FF.
    uint8_t index = i & 0x3F;
    return NES_PALETTE[index];
}

Ppu* init_ppu() {
    Ppu* ppu = (Ppu*)malloc(sizeof(Ppu));

    ppu->cart = NULL;
    memset(&ppu->framebuffer, 0x00000000, PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT);
    memset(&ppu->name_table, 0, sizeof(ppu->name_table));
    memset(&ppu->palette_table, 0, sizeof(ppu->palette_table));
    memset(&ppu->patternTable, 0, sizeof(ppu->patternTable));

    ppu->scanline = 0;
    ppu->cycle = 0;

    ppu->registers.ctrl = (PpuCtrl){0};
    ppu->registers.status = (PpuStatus){0};
    ppu->registers.mask = (PpuMask){0};

    ppu->vramAddr = (LoopyRegister){0};
    ppu->vramAddr.reg = 0x0000;
    ppu->tramAddr = (LoopyRegister){0};
    ppu->tramAddr.reg = 0x0000;

    ppu->addressLatch = 0x00;
    ppu->ppuDataBuffer = 0x00;
    ppu->fineX = 0x00;
    ppu->bgNextTileId = 0x00;
    ppu->bgNextTileAttr = 0x00;
    ppu->bgNextTileLsb = 0x00;
    ppu->bgNextTileMsb = 0x00;
    ppu->bgShifterPatternLo = 0x0000;
    ppu->bgShifterPatternHi = 0x0000;
    ppu->bgShifterAttribLo = 0x0000;
    ppu->bgShifterAttribHi = 0x0000;

    ppu->frame_done = false;
    ppu->nmi_occurred = false;

    return ppu;
}

void ppu_connect_cart(Ppu* ppu, Cartridge* cart) {
    ppu->cart = cart;
}


void ppu_increment_scroll_x(Ppu* ppu) {
    if (ppu->registers.mask.bits.renderBackground || ppu->registers.mask.bits.renderSprites) {
        if (ppu->vramAddr.bits.coarseX == 31) {
            ppu->vramAddr.bits.coarseX = 0;
            ppu->vramAddr.bits.nametableX = ~ppu->vramAddr.bits.nametableX;
        }
        else {
            ppu->vramAddr.bits.coarseX++;
        }
    }
}

void ppu_increment_scroll_y(Ppu* ppu) {
    if (ppu->registers.mask.bits.renderBackground || ppu->registers.mask.bits.renderSprites) {
        if (ppu->vramAddr.bits.fineY < 7) {
            ppu->vramAddr.bits.fineY++;
        }
        else {
            ppu->vramAddr.bits.fineY = 0;
            if (ppu->vramAddr.bits.coarseY == 29) {
                ppu->vramAddr.bits.coarseY = 0;
                ppu->vramAddr.bits.nametableY = ~ppu->vramAddr.bits.nametableY;
            }
            else if (ppu->vramAddr.bits.coarseY == 31) {
                ppu->vramAddr.bits.coarseY = 0;
            }
            else {
                ppu->vramAddr.bits.coarseY++;
            }
        }
    }
}

void ppu_transfer_address_x(Ppu* ppu) {
    if (ppu->registers.mask.bits.renderBackground || ppu->registers.mask.bits.renderSprites) {
        ppu->vramAddr.bits.nametableX = ppu->tramAddr.bits.nametableX;
        ppu->vramAddr.bits.coarseX = ppu->tramAddr.bits.coarseX;
    }
}

void ppu_transfer_address_y(Ppu* ppu) {
    if (ppu->registers.mask.bits.renderBackground || ppu->registers.mask.bits.renderSprites) {
        ppu->vramAddr.bits.fineY = ppu->tramAddr.bits.fineY;
        ppu->vramAddr.bits.nametableY = ppu->tramAddr.bits.nametableY;
        ppu->vramAddr.bits.coarseY = ppu->tramAddr.bits.coarseY;
    }
}

void ppu_load_bg_shifters(Ppu* ppu) {
    ppu->bgShifterPatternLo = (ppu->bgShifterPatternLo & 0xFF00) | ppu->bgNextTileLsb;
    ppu->bgShifterPatternHi = (ppu->bgShifterPatternHi & 0xFF00) | ppu->bgNextTileMsb;
    ppu->bgShifterAttribLo = (ppu->bgShifterAttribLo & 0xFF00) | ((ppu->bgNextTileAttr & 0b01) ? 0xFF : 0x00);
    ppu->bgShifterAttribHi = (ppu->bgShifterAttribHi & 0xFF00) | ((ppu->bgNextTileAttr & 0b10) ? 0xFF : 0x00);
}

void ppu_update_shifters(Ppu* ppu) {
    if (ppu->registers.mask.bits.renderBackground) {
        ppu->bgShifterPatternLo <<= 1;
        ppu->bgShifterPatternHi <<= 1;
        ppu->bgShifterAttribLo <<= 1;
        ppu->bgShifterAttribHi <<= 1;
    }
}

void ppu_clock (Ppu* ppu) {
    if (ppu->scanline >= -1 && ppu->scanline < 240) {
        if (ppu->scanline == 0 && ppu->cycle == 0) {
            ppu->cycle = 1;
        }
        if (ppu->scanline == -1 && ppu->cycle == 1) {
            ppu->registers.status.bits.verticalBlank = 0;
        }
        if ((ppu->cycle >= 2 && ppu->cycle < 258) || (ppu->cycle >= 321 && ppu->cycle < 338)) {
            ppu_update_shifters(ppu);

            switch ((ppu->cycle - 1) % 8) {
                case 0:
                    ppu_load_bg_shifters(ppu);
                    ppu->bgNextTileId = ppu_read(ppu, 0x2000 | (ppu->vramAddr.reg & 0x0FFF));
                    break;
                case 2:
                    ppu->bgNextTileAttr = ppu_read(ppu, 0x23C0 | (ppu->vramAddr.bits.nametableY << 11)
                        | (ppu->vramAddr.bits.nametableX << 10)
                        | ((ppu->vramAddr.bits.coarseY >> 2) << 3)
                        | (ppu->vramAddr.bits.coarseX >> 2)); 
                    if (ppu->vramAddr.bits.coarseY & 0x02) ppu->bgNextTileAttr >>= 4;
                    if (ppu->vramAddr.bits.coarseX & 0x02) ppu->bgNextTileAttr >>= 2;
                    ppu->bgNextTileAttr &= 0x03;
                    break;
                case 4:
                    ppu->bgNextTileLsb = ppu_read(ppu, (ppu->registers.ctrl.bits.patternBackground << 12)
                        + ((uint16_t)ppu->bgNextTileId << 4)
                        + (ppu->vramAddr.bits.fineY) + 0);
                    break;
                case 6:
                    ppu->bgNextTileMsb = ppu_read(ppu, (ppu->registers.ctrl.bits.patternBackground << 12)
                        + ((uint16_t)ppu->bgNextTileId << 4)
                        + (ppu->vramAddr.bits.fineY) + 8);
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
            ppu->bgNextTileId = ppu_read(ppu, (0x2000 | (ppu->vramAddr.reg & 0x0FFF)));
        }
        if (ppu->scanline == -1 && ppu->cycle >= 280 && ppu->cycle < 305) {
            ppu_transfer_address_y(ppu);
        }
    }

    if (ppu->scanline == 240) {
        // Nothing happen here
    }

    if (ppu->scanline >= 241 && ppu->scanline < 261) {
        if (ppu->scanline == 241 && ppu->cycle == 1) {
            ppu->registers.status.bits.verticalBlank = 1;
            if (ppu->registers.ctrl.bits.enableNmi) {
                ppu->nmi_occurred = true;
            }
        }
    }

    uint8_t bgPixel = 0x00;
    uint8_t bgPalette = 0x00;

    if (ppu->registers.mask.bits.renderBackground) {
        uint16_t bitMux = 0x8000 >> ppu->fineX;
        uint8_t p0Pixel = (ppu->bgShifterPatternLo & bitMux) > 0;
        uint8_t p1Pixel = (ppu->bgShifterPatternHi & bitMux) > 0;
        bgPixel = (p1Pixel << 1) | p0Pixel;

        uint8_t bgPalette0 = (ppu->bgShifterAttribLo & bitMux) > 0;
        uint8_t bgPalette1 = (ppu->bgShifterAttribHi & bitMux) > 0;
        bgPalette = (bgPalette1 << 1) | bgPalette0;
    }

    // Update the framebuffer with the appropiate pixels + palettes
    if ((ppu->cycle-1) >= 0 && (ppu->cycle-1) < PPU_SCREEN_WIDTH && ppu->scanline >= 0 && ppu->scanline < PPU_SCREEN_HEIGHT) {
        ppu->framebuffer[((ppu->cycle - 1) * PPU_SCREEN_HEIGHT) + ppu->scanline] = (get_palette_colour(ppu_read(ppu, 0x3F00 + (bgPalette << 2) + bgPixel)));
    }
    
    // printf("Cycle: %d\n", ppu->cycle);
    // printf("Scanline: %d\n", ppu->scanline);

    // Advance renderer - it never stops, it's relentless
    ppu->cycle++;
    if (ppu->cycle >= 341) {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline >= 261) {
            ppu->scanline = -1;
            ppu->frame_done = true;
        }
    }
}


// Ability for CPU to read/write PPU registers
uint8_t cpu_ppu_read(Ppu* ppu, uint16_t address, bool rdOnly) {
    uint8_t data = 0x00;
    if (rdOnly) {
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
                data = (ppu->registers.status.reg & 0xE0) | (ppu->ppuDataBuffer & 0x1F);
                ppu->registers.status.bits.verticalBlank = 0;
                ppu->addressLatch = 0;
                break;

            // OAM Address
            case 0x0003: break;

            // OAM Data
            case 0x0004: break;

            // Scroll - Not Readable
            case 0x0005: break;

            // PPU Address - Not Readable
            case 0x0006: break;

            // PPU Data
            case 0x0007: 
                data = ppu->ppuDataBuffer;
                ppu->ppuDataBuffer = ppu_read(ppu, ppu->vramAddr.reg);
                if (ppu->vramAddr.reg >= 0x3F00) {
                    data = ppu->ppuDataBuffer;
                }
                ppu->vramAddr.reg += ppu->registers.ctrl.bits.incrementMode ? 32 : 1;
                break;
		}
	}
    return data;
}

void cpu_ppu_write(Ppu* ppu, uint16_t address, uint8_t data) {
    switch (address) {
        case 0x0000: // Control
            ppu->registers.ctrl.reg = data;
            ppu->tramAddr.bits.nametableX = ppu->registers.ctrl.bits.nametableX;
            ppu->tramAddr.bits.nametableY = ppu->registers.ctrl.bits.nametableY;
            break;
        case 0x0001: // Mask
            ppu->registers.mask.reg = data;
            break;
        case 0x0002: // Status
            break;
        case 0x0003: // OAM Address
            break;
        case 0x0004: // OAM Data
            break;
        case 0x0005: // Scroll
            if (ppu->addressLatch == 0) {
                ppu->fineX = data & 0x07;
                ppu->tramAddr.bits.coarseX = data >> 3;
                ppu->addressLatch = 1;
            }
            else {
                ppu->tramAddr.bits.fineY = data & 0x07;
                ppu->tramAddr.bits.coarseY = data >> 3;
                ppu->addressLatch = 0;
            }
            break;
        case 0x0006: // PPU Address
            if (ppu->addressLatch == 0) {
                ppu->tramAddr.reg = (uint16_t)((data & 0x3F) << 8) | (ppu->tramAddr.reg & 0x00FF);
                ppu->addressLatch = 1;
            }
            else {
                ppu->tramAddr.reg = (ppu->tramAddr.reg & 0xFF00) | data;
                ppu->vramAddr = ppu->tramAddr; 
                ppu->addressLatch = 0;
            }
            break;
        case 0x0007: // PPU Data
            ppu_write(ppu, ppu->vramAddr.reg, data);
            ppu->vramAddr.reg += ppu->registers.ctrl.bits.incrementMode ? 32 : 1;
            break;
    }
}


// Ability for PPU to read and write data
uint8_t ppu_read(Ppu* ppu, uint16_t address) {
    uint8_t data = 0x00;
    address &= 0x3FFF;

    if (cartridge_ppu_read(ppu->cart, address, &data)) {
        
    }
    // Pattern table
    else if (address >= 0x0000 && address <= 0x1FFF) {
        // Get the most significant bit of the address and offsets by the rest of the bits of the address
        data = ppu->patternTable[(address & 0x1000) >> 12][address & 0x0FFF];
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

    //printf("returning %04x\n", data);
    return data;
}

void ppu_write(Ppu* ppu, uint16_t address, uint8_t data) {
    address &= 0x3FFF;

    if (cartridge_ppu_write(ppu->cart, address, data)) {

    }
    // Pattern table
    else if (address >= 0x0000 && address <= 0x1FFF) {
        // This memory acts as a ROM for the PPU,
        // but for som NES ROMs, this memory is a RAM.
        ppu->patternTable[(address & 0x1000) >> 12][address & 0x0FFF] = data;
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




