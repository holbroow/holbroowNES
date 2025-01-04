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
    uint8_t OAMDMA;    // ($4014) not directly in PPU reg space, but used by CPU to write OAM
} PpuRegisters;

// Sprite data structure
typedef struct {


} Sprite;

// PPU structure
typedef struct Ppu {


    bool frame_done;
    bool nmi_occured;
} Ppu;



// Init PPU (+ cartridge with said PPU)
Ppu* init_ppu();
void connect_cartridge_ppu(Ppu* ppu, Cartridge* cart);

// 'Clock' the PPU
void ppu_clock(Ppu* ppu);

// Ability for CPU to read/write PPU registers
uint8_t ppu_read(Ppu* ppu, uint16_t address);
void ppu_write(Ppu* ppu, uint16_t address, uint8_t value);


