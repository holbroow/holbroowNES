// Bus.h
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward declaration to avoid circular dependencies
typedef struct Ppu Ppu;
typedef struct Cartridge Cartridge;

/*///////BUS STRUCTURE/////////////////////////////////////////////////////////////////////////////////

BUS - 0x0000 - 0xFFFF
    RAM RANGE - 0x0000 - 0x1FFF
                Mirrored: 0x000 - 0x07FF | 0x800 - 0x0FFF | 0x1000 - 0x17FF | 0x1800 - 0x1FFF
    ROM RANGE - 0x4020 - 0xFFFF

///////////////////////////////////////////////////////////////////////////////////////////////////*/

// Define the memory size for the bus (64KB for 16-bit addressing)
#define BUS_MEMORY_SIZE 65535  // 65535 Bytes of addressable memory
#define ROM_MEMORY_SIZE 49126  // ROM size
#define PPU_MEMORY_SIZE 16384  // PPU memory size (no longer needed in Bus)

// Define the Bus structure
typedef struct Bus {
    uint8_t bus_memory[BUS_MEMORY_SIZE];  // System RAM
    Ppu* ppu;                             // Reference to PPU
    Cartridge* cart;                      // Reference to Cartridge
} Bus;

// Function to initialize the bus
Bus* init_bus();

// Function to write data to the bus
void bus_write(Bus* bus, uint16_t address, uint8_t data);

// Function to read data from the bus
uint8_t bus_read(Bus* bus, uint16_t address);
