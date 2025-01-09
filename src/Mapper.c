// Mapper.c
// Supports use of Mapper 0, 1, 2, 3
// Nintendo Entertainment System Base Mapper Implementation
// Will Holbrook - 3rd January 2025
#include <stdint.h>

#include "Mapper.h"

Mapper *mapper_create(
    uint8_t prg_banks, uint8_t chr_banks,
    uint8_t *mapper1_shift_register,
    uint8_t *mapper1_control,
    uint8_t *mapper1_chr_bank0,
    uint8_t *mapper1_chr_bank1,
    uint8_t *mapper1_prg_bank,
    uint8_t *mapper2_prg_bank_select,
    uint8_t *mapper3_chr_bank_select
) {
    Mapper *new_mapper = (Mapper *)malloc(sizeof(Mapper));

    new_mapper->prg_banks = prg_banks;
    new_mapper->chr_banks = chr_banks;

    // Initialize function pointers to NULL (to be set up later as needed)
    new_mapper->mapper_cpu_read = NULL;
    new_mapper->mapper_cpu_write = NULL;
    new_mapper->mapper_ppu_read = NULL;
    new_mapper->mapper_ppu_write = NULL;

    // Initialize Mapper 1 fields
    new_mapper->mapper1_shift_register = mapper1_shift_register ? *mapper1_shift_register : 0;
    new_mapper->mapper1_control        = mapper1_control        ? *mapper1_control        : 0;
    new_mapper->mapper1_chr_bank0      = mapper1_chr_bank0      ? *mapper1_chr_bank0      : 0;
    new_mapper->mapper1_chr_bank1      = mapper1_chr_bank1      ? *mapper1_chr_bank1      : 0;
    new_mapper->mapper1_prg_bank       = mapper1_prg_bank       ? *mapper1_prg_bank       : 0;

    // Initialize Mapper 2 fields
    new_mapper->mapper2_prg_bank_select = mapper2_prg_bank_select ? *mapper2_prg_bank_select : 0;

    // Initialize Mapper 3 fields
    new_mapper->mapper3_chr_bank_select = mapper3_chr_bank_select ? *mapper3_chr_bank_select : 0;

    return new_mapper;
}
