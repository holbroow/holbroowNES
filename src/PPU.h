#pragma once

#include "Cartridge.h"
#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Screen & Timing Constants
// ---------------------------------------------------------------------------
#define PPU_SCREEN_WIDTH 256
#define PPU_SCREEN_HEIGHT 240

// NTSC Timing: 262 scanlines per frame
//   - Visible: 0-239
//   - Post-render: 240
//   - Vertical blank: 241-260
//   - Pre-render: 261
// Each scanline consists of 341 PPU cycles.


typedef struct Sprite {
    uint32_t *pixels;
    uint16_t width;
    uint16_t height;
} Sprite;


// Control Register (PPUCTRL)
typedef union PpuCtrl {
    struct {
        uint8_t nametable_x       : 1;
        uint8_t nametable_y       : 1;
        uint8_t increment_mode    : 1;
        uint8_t pattern_sprite    : 1;
        uint8_t pattern_background: 1;
        uint8_t sprite_size       : 1;
        uint8_t slave_mode        : 1;
        uint8_t enable_nmi        : 1;
    };
    uint8_t reg;
} PpuCtrl;

// Status Register (PPUSTATUS)
typedef union PpuStatus {
    struct {
        uint8_t unused          : 5;
        uint8_t sprite_overflow : 1;
        uint8_t sprite_zero_hit : 1;
        uint8_t vertical_blank  : 1;
    };
    uint8_t reg;
} PpuStatus;

// Mask Register (PPUMASK)
typedef union PpuMask {
    struct {
        uint8_t grayscale             : 1;
        uint8_t render_background_left: 1;
        uint8_t render_sprites_left   : 1;
        uint8_t render_background     : 1;
        uint8_t render_sprites        : 1;
        uint8_t enhance_red           : 1;
        uint8_t enhance_green         : 1;
        uint8_t enhance_blue          : 1;
    };
    uint8_t reg;
} PpuMask;

// Grouping PPU registers together.
typedef struct PpuRegisters {
    PpuCtrl   ctrl;
    PpuStatus status;
    PpuMask   mask;
} PpuRegisters;


// 'Loopy' Register: For VRAM Addressing & Scrolling (This was a wierd one)
typedef union LoopyRegister {
    struct {
        uint16_t coarse_x    : 5;
        uint16_t coarse_y    : 5;
        uint16_t nametable_x : 1;
        uint16_t nametable_y : 1;
        uint16_t fine_y      : 3;
        uint16_t unused      : 1;
    };
    uint16_t reg;
} LoopyRegister;


typedef struct sObjectAttributeEntry {
    uint8_t y;          // Sprite Y position
    uint8_t id;         // Tile ID within pattern memory
    uint8_t attribute;  // Rendering flags
    uint8_t x;          // Sprite X position
} sObjectAttributeEntry;



// PPU Main Structure

typedef struct Ppu {
    Cartridge* cart;

    // Framebuffer: holds rendered pixels for the screen.
    uint32_t framebuffer[PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT];

    // PPU Memory: Nametables, Pattern Tables, and Palette.
    uint8_t name_table[2][1024];
    uint8_t pattern_table[2][4096]; // Not used in real emulation; kept for design.
    uint8_t palette_table[32];

    // Sprite pointers for screen and internal representations.
    Sprite* spr_screen;
    Sprite* spr_name_table[2];
    Sprite* spr_pattern_table[2];

    // PPU Timing: Current scanline, cycle, and frame count.
    int scanline;
    int cycle;
    int frames_completed;

    // PPU Registers & VRAM Addressing
    PpuRegisters registers;
    LoopyRegister vram_addr;
    LoopyRegister tram_addr;

    // Internal variables for address buffering and background rendering.
    uint8_t address_latch;
    uint8_t ppu_data_buffer;
    uint8_t fine_x;
    uint8_t bg_next_tile_id;
    uint8_t bg_next_tile_attr;
    uint8_t bg_next_tile_lsb;
    uint8_t bg_next_tile_msb;
    uint16_t bg_shifter_pattern_lo;
    uint16_t bg_shifter_pattern_hi;
    uint16_t bg_shifter_attrib_lo;
    uint16_t bg_shifter_attrib_hi;

    // Primary OAM: Sprite data for rendering.
    sObjectAttributeEntry oam[64];
    uint8_t oam_addr;

    // Sprite evaluation for current scanline.
    sObjectAttributeEntry sprite_scanline[8];
    uint8_t sprite_count;
    uint8_t sprite_shifter_pattern_lo[8];
    uint8_t sprite_shifter_pattern_hi[8];

    // Sprite Zero hit flags.
    bool b_sprite_zero_hit_possible;
    bool b_sprite_zero_being_rendered;

    // Secondary OAM pointer for CPU sprite memory (if needed).
    uint8_t* p_oam;

    // Flags indicating frame completion and NMI occurrence.
    bool frame_done;
    bool nmi_occurred;
} Ppu;



// PPU Interface Functions

// Initialization and reset functions.
Ppu* init_ppu();
void ppu_reset(Ppu* ppu);

// PPU clock: advances the PPU by one cycle.
void ppu_clock(Ppu* ppu);

// CPU interface for reading and writing PPU registers.
uint8_t cpu_ppu_read(Ppu* ppu, uint16_t address);
void cpu_ppu_write(Ppu* ppu, uint16_t address, uint8_t data);

// Direct PPU memory access functions.
uint8_t ppu_read(Ppu* ppu, uint16_t address);
void ppu_write(Ppu* ppu, uint16_t address, uint8_t data);
