// Bus.h
#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

// Define the memory size for the bus (64KB for 16-bit addressing)
#define MEMORY_SIZE 65535 // 65535 Bytes of addressable memory, this currently makes up 
                          // the whole bus but i want to make it 'attached' soon to allow for other peripherals.

// Define the Bus structure
typedef struct {
    uint8_t memory[MEMORY_SIZE];
} Bus;

// Function to initialize the bus
Bus* init_bus();

// Function to write data to the bus
void bus_write(Bus* bus, uint16_t address, uint8_t data);
// Function to read data from the bus
uint8_t bus_read(Bus* bus, uint16_t address);

#endif // BUS_H
