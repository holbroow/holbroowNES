// Mapper_2.c
// Nintendo Entertainment System Mapper 2 (UxROM) Implementation
// Will Holbrook - 3rd January 2025
#include <stdint.h>
#include <stdbool.h>

#include "Mapper.h"

// Forward declarations of Mapper 2 specific read/write functions
static bool mapper2_cpu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr);
static bool mapper2_cpu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr);
static bool mapper2_ppu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr);
static bool mapper2_ppu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr);

void mapper_load_2(Mapper *mapper) {
    // Set the function pointers to the Mapper 2 specific implementations
    mapper->mapper_cpu_read = mapper2_cpu_read;
    mapper->mapper_cpu_write = mapper2_cpu_write;
    mapper->mapper_ppu_read = mapper2_ppu_read;
    mapper->mapper_ppu_write = mapper2_ppu_write;

    // Initialize Mapper 2 specific registers
    mapper->mapper2_prg_bank_select = 0;
}

/**
 * CPU Read for Mapper 2.
 * Maps CPU addresses to PRG ROM banks based on UxROM behavior.
 */
static bool mapper2_cpu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    // UxROM maps CPU addresses from 0x8000 to 0xFFFF.
    if (address < 0x8000) {
        return false;
    }

    uint32_t bank = 0;
    uint32_t offset = 0;

    if (address < 0xC000) {
        // 0x8000 - 0xBFFF: Switchable bank
        bank = this->mapper2_prg_bank_select;
        offset = address - 0x8000;
    } else {
        // 0xC000 - 0xFFFF: Fixed to last bank
        bank = this->prg_banks - 1;
        offset = address - 0xC000;
    }

    // Assuming each bank is 16KB
    *mapped_addr = (bank * 0x4000) + offset;
    return true;
}

/**
 * CPU Write for Mapper 2.
 * Writing to 0x8000-0xFFFF updates the PRG bank selection.
 */
static bool mapper2_cpu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    // Writes in 0x8000-0xFFFF select the PRG bank
    if (address < 0x8000) {
        return false;
    }

    // The value written selects the bank.
    // Assuming *mapped_addr contains the value being written.
    uint8_t value = (uint8_t)(*mapped_addr & 0xFF);
    this->mapper2_prg_bank_select = value & (this->prg_banks - 1);
    return true;
}

/**
 * PPU Read for Mapper 2.
 * Direct mapping for CHR addresses, typical for CHR ROM.
 */
static bool mapper2_ppu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    // Mapper 2 maps PPU addresses from 0x0000 to 0x1FFF directly to CHR ROM.
    if (address >= 0x2000) {
        return false;
    }

    // Assuming a direct map: bank 0 of CHR covers the whole range.
    *mapped_addr = address;
    return true;
}

/**
 * PPU Write for Mapper 2.
 * If CHR RAM is present, writes are allowed; otherwise, they are ignored for CHR ROM.
 */
static bool mapper2_ppu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    if (address >= 0x2000) {
        return false;
    }

    // If CHR RAM exists, allow writing.
    // For now, assume CHR is ROM; ignore writes.
    return false;
}
