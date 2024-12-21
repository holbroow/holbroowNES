// Will Holbrook | Lancaster University Third Year Project 2024 (SCC 300: EmuPC)
// Bus.c (Bus, which initalises memory and allows read and writes to said memory, as nothing else is currently attached to the bus.)

#include "Bus.h"

/*///////BUS STRUCTURE/////////////////////////////////////////////////////////////////////////////////

BUS - 0x0000 - 0xFFFF
    RAM RANGE - 0x0000 - 0x1FFF
                Mirrored: 0x000 - 0x07FF | 0x800 - 0x0FFF | 0x1000 - 0x17FF | 0x1800 - 0x1FFF
    ROM RANGE - 0x4020 - 0xFFFF

PPU - 0x0000 - 0xFFFF
    PATTERN - 0x0000 - 0x1FFF
    NAMETABLE - 0x2000 - 0x2FFF
    PALLETES - 0x3F00 - 0x3FFF

/////////////////////////////////////////////////////////////////////////////////////////////////////*/

// Function to initialize the bus
Bus* init_bus() {
    Bus* bus = (Bus*)malloc(sizeof(Bus));
    if (!bus) {
        fprintf(stderr, "BUS: Error, Failed to allocate memory for the Bus.\n");
        exit(1);
    }
    printf("BUS: Bus initialised!\n");

    // Initialize all memory to zero
    // CPU
    for (size_t i = 0; i < BUS_MEMORY_SIZE; i++) {
        bus->bus_memory[i] = 0;
    }
    printf("BUS: 2KB System Memory initialised!\n");

    // ROM
    for (size_t i = 0; i < ROM_MEMORY_SIZE; i++) {
        bus->rom_memory[i] = 0;
    }
    printf("BUS: 2KB System Memory initialised!\n");

    // INIT DONE!
    return bus;
}

// Function to write data to the bus
void bus_write(Bus* bus, uint16_t address, uint8_t data) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        // MAIN MEMORY (CPU MEMORY)
        // The & 0x07FF ensures that the address resolves to
        // 0x0000 - 0x07FF, meaning it always works with the 3 memory mirrors
        bus->bus_memory[address & 0x07FF] = data;

    } else if (address >= 0x2000 && address <= 0x3FFF) {
        // PPU - mirrored every 8 (addr & 0x0007)
        bus->ppu_memory[address & 0x0007] = data;

    } else if (address >= 0x4020 && address <= 0xFFFF) {
        // PROGRAM ROM
        bus->rom_memory[address] = data;

    } else {
        fprintf(stderr, "Error,Adress 0x%04x out of range for write operation.\n", address);
    }
}

// Function to read data from the bus
uint8_t bus_read(Bus* bus, uint16_t address) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        // MAIN MEMORY (CPU MEMORY)
        return bus->bus_memory[address & 0x07FF];

    } else if (address >= 0x2000 && address <= 0x3FFF) {
        // PPU - mirrored every 8 (addr & 0x0007)
        return bus->ppu_memory[address & 0x0007];

    } else if (address >= 0x4020 && address <= 0xFFFF) {
        // PROGRAM ROM
        return bus->bus_memory[address];

    } else {
        fprintf(stderr, "Error: Address 0x%04x out of range for read operation.\n", address);
        return 0; // Return 0 or handle as appropriate
    }
}
