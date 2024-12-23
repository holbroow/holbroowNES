#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// Define constants for maximum sizes (adjust as needed)
#define MAX_PRG_ROM_SIZE (256 * 16 * 1024) // 256 banks * 16KB
#define MAX_CHR_ROM_SIZE (256 * 8 * 1024)  // 256 banks * 8KB
#define TRAINER_SIZE 512

typedef struct Cartridge {
    // Header information
    char magic[4];           // "NES\x1A"
    uint8_t prg_rom_banks;   // Number of 16KB PRG ROM banks
    uint8_t chr_rom_banks;   // Number of 8KB CHR ROM banks
    uint8_t flags6;          // Flags 6
    uint8_t flags7;          // Flags 7
    uint8_t flags8;          // Flags 8 (NES 2.0)
    uint8_t flags9;          // Flags 9 (NES 2.0)
    uint8_t flags10;         // Flags 10 (NES 2.0)
    uint8_t padding[5];      // Padding bytes 11-15

    // Mapper information
    uint8_t mapper;          // Mapper number
    bool has_trainer;        // Trainer presence
    bool battery;            // Battery-backed RAM
    bool mirroring;          // 0: horizontal, 1: vertical
    bool four_screen;        // Four-screen mirroring

    // Trainer data (optional)
    uint8_t trainer[TRAINER_SIZE];

    // PRG ROM data
    size_t prg_rom_size;
    uint8_t* prg_rom;

    // CHR ROM data
    size_t chr_rom_size;
    uint8_t* chr_rom;

    // NES 2.0 flag
    bool nes2;                // Indicates if NES 2.0 format is used

} Cartridge;

// Function prototypes
Cartridge* init_cart(const char* filepath);
void free_cart(Cartridge* cart);
