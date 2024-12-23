#include "Bus.h"
#include "PPU.h"
#include "Cartridge.h"

Bus* init_bus() {
    Bus* bus = (Bus*)malloc(sizeof(Bus));
    if (!bus) {
        fprintf(stderr, "[BUS] Error, Failed to allocate memory for the Bus.\n");
        exit(1);
    }
    printf("[BUS] Bus initialized!\n");

    // Initialize all system RAM to zero
    memset(bus->bus_memory, 0, BUS_MEMORY_SIZE);
    printf("[BUS] System Memory initialized!\n");

    // Initialize PPU and Cartridge pointer to NULL
    bus->ppu = NULL;
    bus->cart = NULL;

    return bus;
}

void free_bus(Bus* bus) {
    if (!bus) return;
    free(bus);
}

void bus_write(Bus* bus, uint16_t address, uint8_t data) {
    if (address <= 0x1FFF) {
        // MAIN MEMORY (Mirrored every 0x0800 bytes)
        bus->bus_memory[address & 0x07FF] = data;

    } else if (address >= 0x2000 && address <= 0x3FFF) {
        // PPU Registers (Mirrored every 8 bytes)
        if (bus->ppu) {
            ppu_write(bus->ppu, address & 0x2007, data);
        } else {
            fprintf(stderr, "[BUS] PPU not connected. Cannot write to PPU registers.\n");
        }

    } else if (address >= 0x4000 && address <= 0x4017) {
        // APU and I/O Registers (Not implemented)
        // Handle APU and I/O writes here
        // For now, ignore or implement as needed
        // Example:
        // apu_write(address, data);
        // printf("[BUS] Write to APU/I/O not implemented: 0x%04X\n", address);

    } else if (address >= 0x4020 && address <= 0x5FFF) {
        // Expansion ROM (Not commonly used)
        // Handle expansion ROM writes if necessary
        // For now, ignore or implement as needed
        // Example:
        // expansion_write(address, data);
        // printf("[BUS] Write to Expansion ROM not implemented: 0x%04X\n", address);

    } else if (address >= 0x6000 && address <= 0x7FFF) {
        // SRAM (Battery-backed)
        if (bus->cart && bus->cart->battery) {
            // Implement SRAM write if needed
            // Example:
            // sram[address - 0x6000] = data;
            printf("[BUS] SRAM write not implemented.\n");
        } else {
            fprintf(stderr, "[BUS] SRAM write attempted but not present.\n");
        }

    } else if (address >= 0x8000 && address <= 0xFFFF) {
        // PRG ROM (Read-only in most cases)
        // Some mappers allow writing for bank switching
        // Implement mapper-specific write if necessary
        // For Mapper 0 (no bank switching), writes are ignored
        if (bus->cart) {
            uint8_t mapper = bus->cart->mapper;
            if (mapper == 0) {
                // Mapper 0 does not support bank switching; ignore writes
                // Optionally, log ignored writes
                // printf("[BUS] Write to PRG ROM ignored (Mapper 0).\n");
            } else {
                // Handle other mappers here
                // mapper_write(bus->cart, address, data);
                printf("[BUS] Write to PRG ROM for Mapper %d not implemented.\n", mapper);
            }
        } else {
            fprintf(stderr, "[BUS] No cartridge loaded. Cannot write to PRG ROM.\n");
        }

    } else {
        fprintf(stderr, "[BUS] Write to undefined address 0x%04X\n", address);
    }
}

uint8_t bus_read(Bus* bus, uint16_t address) {
    if (address <= 0x1FFF) {
        // MAIN MEMORY (Mirrored every 0x0800 bytes)
        return bus->bus_memory[address & 0x07FF];

    } else if (address >= 0x2000 && address <= 0x3FFF) {
        // PPU Registers (Mirrored every 8 bytes)
        if (bus->ppu) {
            return ppu_read(bus->ppu, address & 0x2007);
        } else {
            fprintf(stderr, "[BUS] PPU not connected. Cannot read from PPU registers.\n");
            return 0;
        }

    } else if (address >= 0x4000 && address <= 0x4017) {
        // APU and I/O Registers (Not implemented)
        // Handle APU and I/O reads here
        // For now, return 0 or implement as needed
        // Example:
        // return apu_read(address);
        // printf("[BUS] Read from APU/I/O not implemented: 0x%04X\n", address);
        return 0;

    } else if (address >= 0x4020 && address <= 0x5FFF) {
        // Expansion ROM (Not commonly used)
        // Handle expansion ROM reads if necessary
        // For now, return 0 or implement as needed
        // Example:
        // return expansion_read(address);
        // printf("[BUS] Read from Expansion ROM not implemented: 0x%04X\n", address);
        return 0;

    } else if (address >= 0x6000 && address <= 0x7FFF) {
        // SRAM (Battery-backed)
        if (bus->cart && bus->cart->battery) {
            // Implement SRAM read if needed
            // Example:
            // return sram[address - 0x6000];
            printf("[BUS] SRAM read not implemented.\n");
            return 0;
        } else {
            fprintf(stderr, "[BUS] SRAM read attempted but not present.\n");
            return 0;
        }

    } else if (address >= 0x8000 && address <= 0xFFFF) {
        // PRG ROM
        if (bus->cart) {
            size_t prg_size = bus->cart->prg_rom_size;
            if (prg_size == 0) {
                fprintf(stderr, "[BUS] PRG ROM size is 0.\n");
                return 0;
            }

            // Mapper 0: No bank switching, mirror PRG ROM if smaller than 32KB
            uint16_t prg_addr = address - 0x8000;
            if (prg_size == 16 * 1024) {
                // 16KB PRG ROM mirrored to 32KB address space
                prg_addr %= 16 * 1024;
            } else if (prg_size == 32 * 1024) {
                // 32KB PRG ROM mapped directly
                prg_addr %= 32 * 1024;
            } else {
                // Larger sizes would require bank switching logic
                // For Mapper 0, only support 16KB or 32KB PRG ROM
                fprintf(stderr, "[BUS] Unsupported PRG ROM size: %zu bytes\n", prg_size);
                return 0;
            }

            return bus->cart->prg_rom[prg_addr];
        } else {
            fprintf(stderr, "[BUS] No cartridge loaded. Cannot read PRG ROM.\n");
            return 0;
        }

    } else {
        fprintf(stderr, "[BUS] Read from undefined address 0x%04X\n", address);
        return 0;
    }
}
