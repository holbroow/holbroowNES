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
    ppu->frames_completed = 0;

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

    ppu->oam_addr = 0x00;

    ppu->bSpriteZeroHitPossible = false;
    ppu->bSpriteZeroBeingRendered = false;

    ppu->pOAM = (uint8_t*)ppu->OAM;

    ppu->frame_done = false;
    ppu->nmi_occurred = false;

    return ppu;
}

void ppu_connect_cart(Ppu* ppu, Cartridge* cart) {
    ppu->cart = cart;
}

Sprite* get_pattern_table(Ppu* ppu, uint8_t i, uint8_t palette) {
    for (uint16_t nTileY = 0; nTileY < 16; nTileY++) {
        for (uint16_t nTileX = 0; nTileX < 16; nTileX++) {
            uint16_t nOffset = nTileY * 256 + nTileX * 16;
            // Now loop through 8 rows of 8 pixels (Tile)
            for (uint16_t row = 0; row < 8; row++) {
                uint8_t tile_lsb = ppu_read(ppu, i * 0x1000 + nOffset + row + 0x0000);
                uint8_t tile_msb = ppu_read(ppu, i * 0x1000 + nOffset + row + 0x0008);
                for (uint16_t col = 0; col < 8; col++) {
                    uint8_t pixel = ((tile_lsb & 0x01) << 0) | ((tile_msb & 0x01) << 1);
                    tile_lsb >>= 1; tile_msb >>= 1;

                    uint32_t c = (get_palette_colour(ppu_read(ppu, 0x3F00 + (palette << 2) + pixel)));
                    
                    ppu->sprPatternTable[i]->pixels[
                            (nTileX * 8 + (7 - col)) * 
                            ppu->sprPatternTable[i]->height + 
                            (nTileY * 8 + row)
                        ] = c;

                    // uint32_t x = nTileX * 8 + (7 - col);
                    // uint32_t y = nTileY * 8 + row;

                    // ppu->sprPatternTable[i]->pixels[
                    //     y * ppu->sprPatternTable[i]->width + x
                    // ] = c;
                }
            }
        }
    }
    return ppu->sprPatternTable[i];
}

Sprite* get_name_table(Ppu* ppu, uint8_t i) {
    return ppu->sprNameTable[i];
}

static inline uint8_t flipbyte(uint8_t b)
{
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
    return b;
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

    if (ppu->registers.mask.bits.renderSprites && ppu->cycle >= 1 && ppu->cycle < 258) {
        for (size_t i = 0; i < ppu->sprite_count; i++)
        {
            if (ppu->spriteScanline[i].x > 0) {
				ppu->spriteScanline[i].x--;
			} else {
				ppu->sprite_shifter_pattern_lo[i] <<= 1;
				ppu->sprite_shifter_pattern_hi[i] <<= 1;
			}
        }
    }
}

void ppu_clock (Ppu* ppu) {
    if (ppu->scanline >= -1 && ppu->scanline < 240) {
        if (ppu->scanline == 0 && ppu->cycle == 0 && (ppu->frames_completed % 2 != 0) && (ppu->registers.mask.bits.renderBackground || ppu->registers.mask.bits.renderSprites)) {
            ppu->cycle = 1;
        }
        if (ppu->scanline == -1 && ppu->cycle == 1) {
            ppu->registers.status.bits.verticalBlank = 0;
            ppu->registers.status.bits.spriteOverflow = 0;
            ppu->registers.status.bits.spriteZeroHit = 0;
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

    if (ppu->cycle == 257 && ppu->scanline >= 0) {
        memset(ppu->spriteScanline, 0xFF, 8 * sizeof(sObjectAttributeEntry));
        ppu->sprite_count = 0;

        for (uint8_t i = 0; i < 8; i++) {
            ppu->sprite_shifter_pattern_lo[i] = 0;
            ppu->sprite_shifter_pattern_hi[i] = 0;
        }
        
        uint8_t nOAMEntry = 0;
        ppu->bSpriteZeroHitPossible = false;

        while (nOAMEntry < 64 && ppu->sprite_count < 9)
			{
				int16_t diff = ((int16_t)ppu->scanline - (int16_t)ppu->OAM[nOAMEntry].y);
				
				if (diff >= 0 && diff < (ppu->registers.ctrl.bits.spriteSize ? 16 : 8) && ppu->sprite_count < 8)
				{
					if (ppu->sprite_count < 8)
					{
						// Is this sprite sprite zero?
						if (nOAMEntry == 0)
						{
							// It is, so its possible it may trigger a 
							// sprite zero hit when drawn
							ppu->bSpriteZeroHitPossible = true;
						}

						memcpy(&ppu->spriteScanline[ppu->sprite_count], &ppu->OAM[nOAMEntry], sizeof(sObjectAttributeEntry));						
					}			
					ppu->sprite_count++;
				}
				nOAMEntry++;
			} // End of sprite evaluation for next scanline

			// Set sprite overflow flag
			ppu->registers.status.bits.spriteOverflow = (ppu->sprite_count >= 8);
    }

    if (ppu->cycle == 340) {
        for (uint8_t i = 0; i < ppu->sprite_count; i++) {
            uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
            uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;

            if (!ppu->registers.ctrl.bits.spriteSize) {
                if (!(ppu->spriteScanline[i].attribute & 0x80)) {
                    sprite_pattern_addr_lo = 
                        (ppu->registers.ctrl.bits.patternSprite << 12  )  // Which Pattern Table? 0KB or 4KB offset
                    | (ppu->spriteScanline[i].id   << 4   )  // Which Cell? Tile ID * 16 (16 bytes per tile)
                    | (ppu->scanline - ppu->spriteScanline[i].y); // Which Row in cell? (0->7)
                                            
                } else {
                    // Sprite is flipped vertically, i.e. upside down
                    sprite_pattern_addr_lo = 
                        (ppu->registers.ctrl.bits.patternSprite << 12  )  // Which Pattern Table? 0KB or 4KB offset
                    | (ppu->spriteScanline[i].id   << 4   )  // Which Cell? Tile ID * 16 (16 bytes per tile)
                    | (7 - (ppu->scanline - ppu->spriteScanline[i].y)); // Which Row in cell? (7->0)
                }

            } else {
                // 8x16 Sprite Mode - The sprite attribute determines the pattern table
                if (!(ppu->spriteScanline[i].attribute & 0x80)) {
                    // Sprite is NOT flipped vertically, i.e. normal
                    if (ppu->scanline - ppu->spriteScanline[i].y < 8) {
                        // Reading Top half Tile
                        sprite_pattern_addr_lo = 
                            ((ppu->spriteScanline[i].id & 0x01)      << 12)  // Which Pattern Table? 0KB or 4KB offset
                        | ((ppu->spriteScanline[i].id & 0xFE)      << 4 )  // Which Cell? Tile ID * 16 (16 bytes per tile)
                        | ((ppu->scanline - ppu->spriteScanline[i].y) & 0x07 ); // Which Row in cell? (0->7)
                    } else {
                        // Reading Bottom Half Tile
                        sprite_pattern_addr_lo = 
                            ( (ppu->spriteScanline[i].id & 0x01)      << 12)  // Which Pattern Table? 0KB or 4KB offset
                        | (((ppu->spriteScanline[i].id & 0xFE) + 1) << 4 )  // Which Cell? Tile ID * 16 (16 bytes per tile)
                        | ((ppu->scanline - ppu->spriteScanline[i].y) & 0x07  ); // Which Row in cell? (0->7)
                    }
                } else {
                    // Sprite is flipped vertically, i.e. upside down
                    if (ppu->scanline - ppu->spriteScanline[i].y < 8) {
                        // Reading Top half Tile
                        sprite_pattern_addr_lo = 
                            ( (ppu->spriteScanline[i].id & 0x01)      << 12)    // Which Pattern Table? 0KB or 4KB offset
                        | (((ppu->spriteScanline[i].id & 0xFE) + 1) << 4 )    // Which Cell? Tile ID * 16 (16 bytes per tile)
                        | (7 - (ppu->scanline - ppu->spriteScanline[i].y) & 0x07); // Which Row in cell? (0->7)
                    } else {
                        // Reading Bottom Half Tile
                        sprite_pattern_addr_lo = 
                            ((ppu->spriteScanline[i].id & 0x01)       << 12)    // Which Pattern Table? 0KB or 4KB offset
                        | ((ppu->spriteScanline[i].id & 0xFE)       << 4 )    // Which Cell? Tile ID * 16 (16 bytes per tile)
                        | (7 - (ppu->scanline - ppu->spriteScanline[i].y) & 0x07); // Which Row in cell? (0->7)
                    }
                }
            }

            // Hi bit plane equivalent is always offset by 8 bytes from lo bit plane
            sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;

            // Now we have the address of the sprite patterns, we can read them
            sprite_pattern_bits_lo = ppu_read(ppu, sprite_pattern_addr_lo);
            sprite_pattern_bits_hi = ppu_read(ppu, sprite_pattern_addr_hi);

            // If the sprite is flipped horizontally, we need to flip the 
            // pattern bytes. 
            if (ppu->spriteScanline[i].attribute & 0x40) {
                // Flip Patterns Horizontally
                sprite_pattern_bits_lo = flipbyte(sprite_pattern_bits_lo);
                sprite_pattern_bits_hi = flipbyte(sprite_pattern_bits_hi);
            }

            // Finally! We can load the pattern into our sprite shift registers
            // ready for rendering on the next scanline
            ppu->sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
            ppu->sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
        }
    }

    if (ppu->scanline == 240) {
        // Nothing happens here
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
        if (ppu->registers.mask.bits.renderBackgroundLeft || (ppu->cycle >= 9)) {
            uint16_t bitMux = 0x8000 >> ppu->fineX;
            uint8_t p0Pixel = (ppu->bgShifterPatternLo & bitMux) > 0;
            uint8_t p1Pixel = (ppu->bgShifterPatternHi & bitMux) > 0;
            bgPixel = (p1Pixel << 1) | p0Pixel;

            uint8_t bgPalette0 = (ppu->bgShifterAttribLo & bitMux) > 0;
            uint8_t bgPalette1 = (ppu->bgShifterAttribHi & bitMux) > 0;
            bgPalette = (bgPalette1 << 1) | bgPalette0;
        }
    }


    uint8_t fg_pixel = 0x00;
	uint8_t fg_palette = 0x00; 
	uint8_t fg_priority = 0x00;

    if (ppu->registers.mask.bits.renderSprites)
	{
		// Iterate through all sprites for this scanline. This is to maintain
		// sprite priority. As soon as we find a non transparent pixel of
		// a sprite we can abort
		if (ppu->registers.mask.bits.renderSpritesLeft || (ppu->cycle >= 9))
		{

			ppu->bSpriteZeroBeingRendered = false;

			for (uint8_t i = 0; i < ppu->sprite_count; i++)
			{
				// Scanline cycle has "collided" with sprite, shifters taking over
				if (ppu->spriteScanline[i].x == 0)
				{
					// Note Fine X scrolling does not apply to sprites, the game
					// should maintain their relationship with the background. So
					// we'll just use the MSB of the shifter

					// Determine the pixel value...
					uint8_t fg_pixel_lo = (ppu->sprite_shifter_pattern_lo[i] & 0x80) > 0;
					uint8_t fg_pixel_hi = (ppu->sprite_shifter_pattern_hi[i] & 0x80) > 0;
					fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

					// Extract the palette from the bottom two bits. Recall
					// that foreground palettes are the latter 4 in the 
					// palette memory.
					fg_palette = (ppu->spriteScanline[i].attribute & 0x03) + 0x04;
					fg_priority = (ppu->spriteScanline[i].attribute & 0x20) == 0;

					// If pixel is not transparent, we render it, and dont
					// bother checking the rest because the earlier sprites
					// in the list are higher priority
					if (fg_pixel != 0)
					{
						if (i == 0) // Is this sprite zero?
						{
							ppu->bSpriteZeroBeingRendered = true;
						}

						break;
					}
				}
			}
		}		
	}

	// Now we have a background pixel and a foreground pixel. They need
	// to be combined. It is possible for sprites to go behind background
	// tiles that are not "transparent", yet another neat trick of the PPU
	// that adds complexity for us poor emulator developers...

	uint8_t pixel = 0x00;   // The FINAL Pixel...
	uint8_t palette = 0x00; // The FINAL Palette...

	if (bgPixel == 0 && fg_pixel == 0)
	{
		// The background pixel is transparent
		// The foreground pixel is transparent
		// No winner, draw "background" colour
		pixel = 0x00;
		palette = 0x00;
	}
	else if (bgPixel == 0 && fg_pixel > 0)
	{
		// The background pixel is transparent
		// The foreground pixel is visible
		// Foreground wins!
		pixel = fg_pixel;
		palette = fg_palette;
	}
	else if (bgPixel > 0 && fg_pixel == 0)
	{
		// The background pixel is visible
		// The foreground pixel is transparent
		// Background wins!
		pixel = bgPixel;
		palette = bgPalette;
	}
	else if (bgPixel > 0 && fg_pixel > 0)
	{
		// The background pixel is visible
		// The foreground pixel is visible
		// Hmmm...
		if (fg_priority)
		{
			// Foreground cheats its way to victory!
			pixel = fg_pixel;
			palette = fg_palette;
		}
		else
		{
			// Background is considered more important!
			pixel = bgPixel;
			palette = bgPalette;
		}

		// Sprite Zero Hit detection
		if (ppu->bSpriteZeroHitPossible && ppu->bSpriteZeroBeingRendered)
		{
			// Sprite zero is a collision between foreground and background
			// so they must both be enabled
			if (ppu->registers.mask.bits.renderBackground & ppu->registers.mask.bits.renderSprites)
			{
				// The left edge of the screen has specific switches to control
				// its appearance. This is used to smooth inconsistencies when
				// scrolling (since sprites x coord must be >= 0)
				if (!(ppu->registers.mask.bits.renderBackgroundLeft |ppu->registers.mask.bits.renderSpritesLeft))
				{
					if (ppu->cycle >= 9 && ppu->cycle < 258)
					{
						ppu->registers.status.bits.spriteZeroHit = 1;
					}
				}
				else
				{
					if (ppu->cycle >= 1 && ppu->cycle < 258)
					{
						ppu->registers.status.bits.spriteZeroHit = 1;
					}
				}
			}
		}
	}


    // Update the framebuffer with the appropiate pixels + palettes
    if ((ppu->cycle-1) >= 0 && (ppu->cycle-1) < PPU_SCREEN_WIDTH && ppu->scanline >= 0 && ppu->scanline < PPU_SCREEN_HEIGHT) {
        //ppu->framebuffer[((ppu->cycle - 1) * PPU_SCREEN_HEIGHT) + ppu->scanline] = (get_palette_colour(ppu_read(ppu, 0x3F00 + (bgPalette << 2) + bgPixel)));
        // ppu->framebuffer[ppu->scanline * PPU_SCREEN_WIDTH + (ppu->cycle - 1)] = (get_palette_colour(ppu_read(ppu, 0x3F00 + (bgPalette << 2) + bgPixel)));
        ppu->framebuffer[ppu->scanline * PPU_SCREEN_WIDTH + (ppu->cycle - 1)] =
            get_palette_colour(
                ppu_read(ppu, 0x3F00 + (palette << 2) + pixel)
            );
    }
    
    // printf("Cycle: %d\n", ppu->cycle);
    // printf("Scanline: %d\n", ppu->scanline);

    // Advance the 'renderer'
    ppu->cycle++;
    if (ppu->registers.mask.bits.renderBackground || ppu->registers.mask.bits.renderSprites) {
        if (ppu->cycle == 260 && ppu->scanline < 240)
		{
			// ppu->cart->GetMapper()->scanline();
            // ppu->cart->

            // TODO: NOT IMPLEMENTED YET, I SHOULD FOR MAPPER 4/5, i forget...
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




