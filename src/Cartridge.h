// Cartridge.h
// Nintendo Entertainment System Cartridge Implementation (Header File)
// Will Holbrook - 20th December 2024
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "Mapper.h"

// Data Structures
typedef struct Vector {
    uint8_t *items;
    size_t size;
    size_t capacity;
} Vector;

typedef enum Mirror {
    HORIZONTAL,
    VERTICAL
} Mirror;

typedef struct Cartridge {
    Vector *prg_memory;
    Vector *chr_memory;
    uint8_t n_prg_banks;
    uint8_t n_chr_banks;
    uint8_t mapper_id;
    Mapper *mapper;
    Mirror mirror;
} Cartridge;

// Initialise the cartridge (using a '.nes' ROM file)
Cartridge* init_cart(const char* filepath);

// CPU Read/Write
bool cartridge_cpu_read(Cartridge *cartridge, uint16_t address, uint8_t* data);
bool cartridge_cpu_write(Cartridge *cartridge, uint16_t address, uint8_t data);

// PPU Read/Write
bool cartridge_ppu_read(Cartridge *cartridge, uint16_t address, uint8_t* data);
bool cartridge_ppu_write(Cartridge *cartridge, uint16_t address, uint8_t data);
