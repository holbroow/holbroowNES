// Mapper_1.c
// Nintendo Entertainment System Mapper 1 (MMC1) Implementation
// Will Holbrook - 3rd January 2025
#include <stdint.h>
#include <stdbool.h>

#include "Mapper.h"

// Forward declarations of Mapper 1 specific read/write functions
static bool mapper1_cpu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr);
static bool mapper1_cpu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr);
static bool mapper1_ppu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr);
static bool mapper1_ppu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr);

void mapper_load_1(Mapper *mapper) {
    // Set the function pointers to the Mapper 1 specific implementations
    mapper->mapper_cpu_read = mapper1_cpu_read;
    mapper->mapper_cpu_write = mapper1_cpu_write;
    mapper->mapper_ppu_read = mapper1_ppu_read;
    mapper->mapper_ppu_write = mapper1_ppu_write;

    // Initialize Mapper 1 specific registers to their default states
    mapper->mapper1_shift_register = 0x10;  // Shift register reset value
    mapper->mapper1_control        = 0x0C;  // Default control value on reset
    mapper->mapper1_chr_bank0      = 0;
    mapper->mapper1_chr_bank1      = 0;
    mapper->mapper1_prg_bank       = 0;
}

/**
 * CPU Read for Mapper 1.
 * This function maps CPU addresses to PRG ROM banks based on MMC1 registers.
 */
static bool mapper1_cpu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    // Mapper 1 typically maps addresses in the range 0x8000-0xFFFF
    if (address < 0x8000) {
        return false;
    }

    // Example logic for mapping CPU read addresses using mapper1_control and mapper1_prg_bank:
    // The actual mapping logic will depend on the control register mode.
    // This simplistic example assumes a fixed bank size mapping.

    uint32_t bank = this->mapper1_prg_bank; 
    uint32_t offset = address & 0x7FFF; // Offset within the 32KB bank

    // Compute mapped address (this is a placeholder; actual computation depends on PRG ROM layout)
    *mapped_addr = (bank * 0x8000) + offset;
    return true;
}

/**
 * CPU Write for Mapper 1.
 * Handles serial loading of MMC1 registers from CPU writes.
 */
static bool mapper1_cpu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    // Mapper 1 registers are typically accessed in the 0x8000-0xFFFF range
    if (address < 0x8000) {
        return false;
    }

    // If the highest bit of the value is set, reset the shift register and update control.
    // This is part of the MMC1 reset mechanism.
    // For simplicity, assume *mapped_addr holds the value being written.
    // In a real scenario, you'd pass the written value as an additional parameter.
    uint8_t value = (uint8_t)(*mapped_addr & 0xFF);

    if (value & 0x80) {
        // Reset shift register
        this->mapper1_shift_register = 0x10;
        // Set control register bits 2-4 to initial state (assuming fixed behavior)
        this->mapper1_control |= 0x0C;
        return true;
    }

    // Shift the register right by one and insert the new bit
    bool complete = (this->mapper1_shift_register & 1);
    this->mapper1_shift_register >>= 1;
    this->mapper1_shift_register |= ((value & 1) << 4);

    if (complete) {
        // Determine target register based on address ranges:
        // 0x8000-0x9FFF -> Control
        // 0xA000-0xBFFF -> CHR Bank 0
        // 0xC000-0xDFFF -> CHR Bank 1
        // 0xE000-0xFFFF -> PRG Bank

        uint8_t reg_value = this->mapper1_shift_register;
        if (address < 0xA000) {
            this->mapper1_control = reg_value;
        } else if (address < 0xC000) {
            this->mapper1_chr_bank0 = reg_value;
        } else if (address < 0xE000) {
            this->mapper1_chr_bank1 = reg_value;
        } else {
            this->mapper1_prg_bank = reg_value;
        }
        // Reset shift register after writing a complete value
        this->mapper1_shift_register = 0x10;
    }
    return true;
}

/**
 * PPU Read for Mapper 1.
 * Maps PPU addresses to CHR ROM/RAM banks based on MMC1 registers.
 */
static bool mapper1_ppu_read(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    // Mapper 1 typically maps addresses in the range 0x0000-0x1FFF for CHR
    if (address >= 0x2000) {
        return false;
    }

    // Example logic for mapping PPU read addresses using CHR bank registers.
    // The actual mapping logic depends on the MMC1 configuration in mapper1_control.
    uint32_t bank = (address < 0x1000) ? this->mapper1_chr_bank0 : this->mapper1_chr_bank1;
    uint32_t offset = address & 0x0FFF; // Offset within a 4KB bank

    // Compute mapped address (placeholder logic; adjust based on CHR ROM layout)
    *mapped_addr = (bank * 0x1000) + offset;
    return true;
}

/**
 * PPU Write for Mapper 1.
 * For CHR RAM writes, this handles writes to CHR space when CHR RAM is present.
 */
static bool mapper1_ppu_write(Mapper *this, uint16_t address, uint32_t *mapped_addr) {
    // If using CHR ROM, writes typically do nothing. If using CHR RAM, implement write logic.
    if (address >= 0x2000) {
        return false;
    }

    // If using CHR RAM, map PPU writes similarly to reads:
    uint32_t bank = (address < 0x1000) ? this->mapper1_chr_bank0 : this->mapper1_chr_bank1;
    uint32_t offset = address & 0x0FFF;

    *mapped_addr = (bank * 0x1000) + offset;
    return true;
}
