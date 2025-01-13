// Mapper.h
// Supports use of Mapper 0, 1, 2, 3
// Nintendo Entertainment System Base Mapper Implementation (Header File)
// Will Holbrook - 3rd January 2025
#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Mapper {
    uint8_t prg_banks;
    uint8_t chr_banks;

    // Function pointers for CPU and PPU read/write operations
    bool (*mapper_cpu_read)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);
    bool (*mapper_cpu_write)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);
    bool (*mapper_ppu_read)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);
    bool (*mapper_ppu_write)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);

    // --- Mapper 1 (MMC1) specific fields ---
    uint8_t mapper1_shift_register;
    uint8_t mapper1_control;
    uint8_t mapper1_chr_bank0;
    uint8_t mapper1_chr_bank1;
    uint8_t mapper1_prg_bank;

    // --- Mapper 2 (UxROM) specific fields ---
    uint8_t mapper2_prg_bank_select;

    // --- Mapper 3 (CNROM) specific fields ---
    uint8_t mapper3_chr_bank_select;

} Mapper;

/**
 * Create a new Mapper instance with optional parameters for Mapper 1, 2, & 3 fields.
 * Pass NULL for any mapper-specific parameters that are not needed.
 */
Mapper *mapper_create(
    uint8_t prg_banks, uint8_t chr_banks,
    uint8_t *mapper1_shift_register,
    uint8_t *mapper1_control,
    uint8_t *mapper1_chr_bank0,
    uint8_t *mapper1_chr_bank1,
    uint8_t *mapper1_prg_bank,
    uint8_t *mapper2_prg_bank_select,
    uint8_t *mapper3_chr_bank_select
);

void mapper_load_0(Mapper *mapper);

void mapper_load_1(Mapper *mapper);

void mapper_load_2(Mapper *mapper);