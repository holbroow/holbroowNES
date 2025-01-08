#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "Mapper.h"

// Define constants for maximum sizes (adjust as needed)
#define MAX_PRG_ROM_SIZE (256 * 16 * 1024) // 256 banks * 16KB
#define MAX_CHR_ROM_SIZE (256 * 8 * 1024)  // 256 banks * 8KB
#define TRAINER_SIZE 512

typedef struct Vector {
    uint8_t *items;
    size_t size;
    size_t capacity;
} Vector;

typedef enum Mirror {
    HORIZONTAL = 0,
    VERTICAL
} Mirror;

typedef struct Cartridge {
    Vector *prg_memory;
    Vector *chr_memory;
    uint8_t mapper_id;
    uint8_t n_prg_banks;
    uint8_t n_chr_banks;
    Mapper *mapper;
    Mirror mirror;
} Cartridge;

// Function prototypes
Cartridge* init_cart(const char* filepath);

bool cartridge_cpu_read(Cartridge *cartridge, uint16_t address, uint8_t* data);
bool cartridge_cpu_write(Cartridge *cartridge, uint16_t address, uint8_t data);

bool cartridge_ppu_read(Cartridge *cartridge, uint16_t address, uint8_t* data);
bool cartridge_ppu_write(Cartridge *cartridge, uint16_t address, uint8_t data);
