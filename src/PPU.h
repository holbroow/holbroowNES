// PPU.h
// Nintendo Entertainment System PPU Implementation (2C02) (Header File)
// Will Holbrook - 1st December 2024
#pragma once

#include "Cartridge.h"

#include <stdint.h>
#include <stdbool.h>

// Screen dimensions
#define PPU_SCREEN_WIDTH  256
#define PPU_SCREEN_HEIGHT 240

// Sprite data structure
typedef struct Sprite {
    uint32_t *pixels;
    uint16_t width;
    uint16_t height;
} Sprite;

// PPU control register
typedef union PpuCtrl {
    struct {
        uint8_t nametable_x         : 1;
        uint8_t nametable_y         : 1;
        uint8_t increment_mode      : 1;
        uint8_t pattern_sprite      : 1;
        uint8_t pattern_background  : 1;
        uint8_t sprite_size         : 1;
        uint8_t slave_mode          : 1;
        uint8_t enable_nmi          : 1;
    };
    uint8_t reg;
} PpuCtrl;

// PPU status register
typedef union PpuStatus {
    struct {
        uint8_t unused           : 5;
        uint8_t sprite_overflow  : 1;
        uint8_t sprite_zero_hit  : 1;
        uint8_t vertical_blank   : 1;
    };
    uint8_t reg;
} PpuStatus;

// PPU mask register
typedef union PpuMask {
    struct {
        uint8_t grayscale               : 1;
        uint8_t render_background_left  : 1;
        uint8_t render_sprites_left     : 1;
        uint8_t render_background       : 1;
        uint8_t render_sprites          : 1;
        uint8_t enhance_red             : 1;
        uint8_t enhance_green           : 1;
        uint8_t enhance_blue            : 1;
    };
    uint8_t reg;
} PpuMask;

// Consolidated PPU registers
typedef struct PpuRegisters {
    PpuCtrl   ctrl;
    PpuStatus status;
    PpuMask   mask;
} PpuRegisters;

// Loopy register (VRAM address)
// (Loopy is the common name given as 'Loopy' was the one to decode and undertand it's concept :0).)
typedef union LoopyRegister {
    struct {
        uint16_t coarse_x       : 5;
        uint16_t coarse_y       : 5;
        uint16_t nametable_x    : 1;
        uint16_t nametable_y    : 1;
        uint16_t fine_y         : 3;
        uint16_t unused         : 1;
    };
    uint16_t reg;
} LoopyRegister;

// Object attribute entry for sprites
typedef struct sObjectAttributeEntry {
    uint8_t y;
    uint8_t id;
    uint8_t attribute;
    uint8_t x;
} sObjectAttributeEntry;

// PPU state
typedef struct Ppu {
    Cartridge* cart;

    uint32_t framebuffer[PPU_SCREEN_WIDTH * PPU_SCREEN_HEIGHT];

    uint8_t name_table[2][1024];
    uint8_t pattern_table[2][4096];
    uint8_t palette_table[32];

    Sprite* spr_screen;
    Sprite* spr_name_table[2];
    Sprite* spr_pattern_table[2];

    int scanline;
    int cycle;
    int frames_completed;

    PpuRegisters   registers;
    LoopyRegister  vram_addr;
    LoopyRegister  tram_addr;

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

    sObjectAttributeEntry oam[64];
    uint8_t oam_addr;

    sObjectAttributeEntry sprite_scanline[8];
    uint8_t sprite_count;
    uint8_t sprite_shifter_pattern_lo[8];
    uint8_t sprite_shifter_pattern_hi[8];

    bool b_sprite_zero_hit_possible;
    bool b_sprite_zero_being_rendered;
    bool frame_done;
    bool nmi_occurred;

    uint8_t* p_oam;
} Ppu;

// API
Ppu*    init_ppu();
void    ppu_connect_cart(Ppu* ppu, Cartridge* cart);
void    ppu_reset(Ppu* ppu);
void    ppu_clock(Ppu* ppu);

// CPU read/write of PPU registers
uint8_t cpu_ppu_read(Ppu* ppu, uint16_t address, bool rd_only);
void    cpu_ppu_write(Ppu* ppu, uint16_t address, uint8_t data);

// Internal read/write
uint8_t ppu_read(Ppu* ppu, uint16_t address);
void    ppu_write(Ppu* ppu, uint16_t address, uint8_t data);

