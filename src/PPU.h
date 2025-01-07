// PPU.h
#pragma once

#include "Cartridge.h"

#include <stdint.h>
#include <stdbool.h>

// Define screen dimensions
#define PPU_SCREEN_WIDTH 256
#define PPU_SCREEN_HEIGHT 240

// PPU timing constants
// NTSC: 262 scanlines total: 
//   Visible lines: 0-239
//   Post-render line: 240
//   Vertical blank: 241-260
//   Pre-render line: 261
//
// Each line has 341 PPU cycles (0-340)

// Sprite data structure
typedef struct Sprite {
    uint32_t *pixels;
    uint16_t width;
    uint16_t height;
} Sprite;

typedef union PpuCtrl {
    struct {
        uint8_t nametableX : 1;
        uint8_t nametableY : 1;
        uint8_t incrementMode : 1;
        uint8_t patternSprite : 1;
        uint8_t patternBackground : 1;
        uint8_t spriteSize : 1;
        uint8_t slaveMode : 1;   // unused
        uint8_t enableNmi : 1;
    } bits;
    uint8_t reg;
} PpuCtrl;

typedef union PpuStatus {
    struct {
        uint8_t unused : 5;
        uint8_t spriteOverflow : 1;
        uint8_t spriteZeroHit : 1;
        uint8_t verticalBlank : 1;
    } bits;
    uint8_t reg;
} PpuStatus;

typedef union PpuMask {
    struct {
        uint8_t grayscale : 1;
        uint8_t renderBackgroundLeft : 1;
        uint8_t renderSpritesLeft : 1;
        uint8_t renderBackground : 1;
        uint8_t renderSprites : 1;
        uint8_t enhanceRed : 1;
        uint8_t enhanceGreen : 1;
        uint8_t enhanceBlue : 1;
    } bits;
    uint8_t reg;
} PpuMask;

// PPU registers
typedef struct PpuRegisters {
    PpuCtrl ctrl;
    PpuStatus status;
    PpuMask mask;
} PpuRegisters;

typedef union LoopyRegister {
    struct {
        uint16_t coarseX : 5;
        uint16_t coarseY : 5;
        uint16_t nametableX : 1;
        uint16_t nametableY : 1;
        uint16_t fineY : 3;
        uint16_t unused : 1;
    } bits;
    uint16_t reg;
} LoopyRegister;

typedef struct sObjectAttributeEntry {
    uint8_t y;			// Y position of sprite
    uint8_t id;			// ID of tile from pattern memory
    uint8_t attribute;	// Flags define how sprite should be rendered
    uint8_t x;			// X position of sprite
} sObjectAttributeEntry;

// PPU structure
typedef struct Ppu {
    Cartridge* cart;

    uint32_t framebuffer[PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT];

    uint8_t name_table[2][1024];
    uint8_t patternTable[2][4096];   // This table wont be used in the real emulation. Just keep it here for the moment for the design.
    uint8_t palette_table[64];

    Sprite* sprScreen;
	Sprite* sprNameTable[2];
	Sprite* sprPatternTable[2];

    int scanline;
    int cycle;
    int frames_completed;

    PpuRegisters registers;
    LoopyRegister vramAddr;
    LoopyRegister tramAddr;

    uint8_t addressLatch;
    uint8_t ppuDataBuffer;
    uint8_t fineX;
    uint8_t bgNextTileId;
    uint8_t bgNextTileAttr;
    uint8_t bgNextTileLsb;
    uint8_t bgNextTileMsb;
    uint16_t bgShifterPatternLo;
    uint16_t bgShifterPatternHi;
    uint16_t bgShifterAttribLo;
    uint16_t bgShifterAttribHi;

    sObjectAttributeEntry OAM[64];
    uint8_t oam_addr;

    sObjectAttributeEntry spriteScanline[8];
	uint8_t sprite_count;
	uint8_t sprite_shifter_pattern_lo[8];
	uint8_t sprite_shifter_pattern_hi[8];

    bool bSpriteZeroHitPossible;
	bool bSpriteZeroBeingRendered;

    uint8_t* pOAM;

    bool frame_done;
    bool nmi_occurred;
} Ppu;


// Init PPU (+ cartridge with said PPU)
Ppu* init_ppu();
void ppu_connect_cart(Ppu* ppu, Cartridge* cart);

// 'Clock' the PPU
void ppu_clock(Ppu* ppu);

// Ability for CPU to read/write PPU registers
uint8_t cpu_ppu_read(Ppu* ppu, uint16_t address, bool rdOnly);
void cpu_ppu_write(Ppu* ppu, uint16_t address, uint8_t data);

// Ability for PPU to read and write data
uint8_t ppu_read(Ppu* ppu, uint16_t address);
void ppu_write(Ppu* ppu, uint16_t address, uint8_t data);

Sprite *GetPatternTable(uint8_t i, uint8_t palette);
