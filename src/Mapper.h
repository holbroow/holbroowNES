#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declaration of the Mapper structure.
typedef struct Mapper Mapper;

// Function pointer typedefs for memory mapping operations.
typedef bool (*MapperReadFunc)(Mapper *mapper, uint16_t address, uint32_t *mapped_addr);
typedef bool (*MapperWriteFunc)(Mapper *mapper, uint16_t address, uint32_t *mapped_addr);

// Base Mapper structure definition.
// This structure supports Mapper 0, 1, 2, and 3.
struct Mapper {
    uint8_t prg_banks;
    uint8_t chr_banks;

    // Function pointers for CPU memory operations.
    MapperReadFunc  mapper_cpu_read;
    MapperWriteFunc mapper_cpu_write;

    // Function pointers for PPU memory operations.
    MapperReadFunc  mapper_ppu_read;
    MapperWriteFunc mapper_ppu_write;

    // --- Mapper 1 (MMC1) specific fields ---
    uint8_t mapper1_shift_register;
    uint8_t mapper1_control;
    uint8_t mapper1_chr_bank0;
    uint8_t mapper1_chr_bank1;
    uint8_t mapper1_prg_bank;

    // --- Mapper 2 (UxROM) specific field ---
    uint8_t mapper2_prg_bank_select;

    // --- Mapper 3 (CNROM) specific field ---
    uint8_t mapper3_chr_bank_select;
};

/**
 * Create a new Mapper instance with optional parameters for Mapper 1, 2, & 3 fields.
 * Pass NULL for any mapper-specific parameters that aren't needed (as is the case with NROM, which is simple (the simplest i believe?)).
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
