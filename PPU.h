// PPU.h
#pragma once

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

// PPU registers
typedef struct {
    uint8_t PPUCTRL;   // $2000
    uint8_t PPUMASK;   // $2001
    uint8_t PPUSTATUS; // $2002
    uint8_t OAMADDR;   // $2003
    uint8_t OAMDATA;   // $2004
    uint8_t PPUSCROLL; // $2005 (latches)
    uint8_t PPUADDR;   // $2006 (latches)
    uint8_t PPUDATA;   // $2007
    uint8_t OAMDMA;    // not directly in PPU reg space, but used by CPU to write OAM
} PpuRegisters;

// Sprite data structure
typedef struct {
    uint8_t y;
    uint8_t tile_index;
    uint8_t attributes;
    uint8_t x;
} Sprite;

// PPU structure
typedef struct Ppu {
    // VRAM address registers (15-bit)
    uint16_t v;  // current VRAM address
    uint16_t t;  // temporary VRAM address
    uint8_t  x;  // fine X scroll (3 bits)
    bool w;      // write toggle for PPUSCROLL/PPUADDR

    // OAM
    uint8_t OAM[256];           // Primary OAM
    uint8_t secondary_OAM[32];  // Secondary OAM for sprite evaluation

    // Framebuffer
    uint32_t framebuffer[PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT];

    // Internal PPU memory
    uint8_t pattern_tables[0x2000];   // 0x0000 - 0x1FFF
    uint8_t nametables[0x1000];       // 0x2000 - 0x2FFF
    uint8_t palettes[0x20];            // 0x3F00 - 0x3F1F

    // Latches for fetching background data
    uint8_t nametable_byte;
    uint8_t attribute_byte;
    uint8_t low_pattern_byte;
    uint8_t high_pattern_byte;

    // Shift registers for background
    uint16_t pattern_shift_low;
    uint16_t pattern_shift_high;
    uint16_t attribute_shift_low;
    uint16_t attribute_shift_high;

    // Sprite rendering data
    Sprite sprite_data[8]; // sprite data for current scanline
    uint8_t sprite_count;
    uint8_t sprite_pattern_low[8];
    uint8_t sprite_pattern_high[8];

    bool sprite_zero_hit_possible;
    bool sprite_zero_being_rendered;

    // Cycle, scanline counters
    int cycle;
    int scanline;
    bool frame_done;
    bool nmi_occurred;

    PpuRegisters reg;
} Ppu;

// Function prototypes
Ppu* init_ppu();
void free_ppu(Ppu* ppu);
void ppu_clock(Ppu* ppu);
bool ppu_is_frame_done(Ppu* ppu);
void ppu_clear_frame_done(Ppu* ppu);

// External interface for CPU to read/write PPU registers
uint8_t ppu_read(Ppu* ppu, uint16_t address);
void ppu_write(Ppu* ppu, uint16_t address, uint8_t value);

// For CPU NMI check
bool ppu_nmi_occurred(Ppu* ppu);
void ppu_clear_nmi(Ppu* ppu);

// Get framebuffer
uint32_t* ppu_get_framebuffer(Ppu* ppu);
