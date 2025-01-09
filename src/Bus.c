// Bus.c
// Nintendo Entertainment System Bus Implementation
// Will Holbrook - 20th October 2024
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
    memset(bus->main_memory, 0, sizeof(bus->main_memory));
    printf("[BUS] System Memory initialized!\n");

    // Initialize PPU and Cartridge pointer to NULL
    bus->ppu = NULL;
    bus->cart = NULL;

    bus->dma_page = 0x00;
	bus->dma_addr = 0x00;
    bus->dma_data = 0x00;
    bus->dma_dummy = true;
	bus->dma_transfer = true;

    return bus;
}


void bus_write(Bus* bus, uint16_t address, uint8_t data) {
    if (cartridge_cpu_write(bus->cart, address, data)) {
        // This allows the Cartridge the opportunity to write to the CPU/Main memory if it wants...

    } else if (address >= 0x0000 && address <= 0x1FFF) {
        // MAIN MEMORY (Mirrored every 0x0800 bytes)
        bus->main_memory[address & 0x07FF] = data;

    } else if (address >= 0x2000 && address <= 0x3FFF) {
        // PPU Registers (Mirrored every 8 bytes)
        cpu_ppu_write(bus->ppu, address & 0x0007, data);

    } else if (address == 0x4014) {
        bus->dma_page = data;
		bus->dma_addr = 0x00;
		bus->dma_transfer = true;

    } else if (address >= 0x4016 && address <= 0x4017) {
        // Controller(s)
        bus->controller_state[address & 0x0001] = bus->controller[address & 0x0001];

    } else if (address >= 0x4020 && address <= 0x5FFF) {
        // Expansion ROM (Not implemented)
        printf("[BUS] Expansion ROM not implemented.\n");

    } else {
        //printf("[BUS] Detected write to undefined address on bus: 0x%04X\n", address);
    }
}


uint8_t bus_read(Bus* bus, uint16_t address) {
    uint8_t data = 0x00;
    if (cartridge_cpu_read(bus->cart, address, &data)) {
        // This allows the Cartridge the opportunity to read from the CPU/Main memory if it wants...
        // We then return the read data after the if/else checks.

    } else if (address >= 0x0000 && address <= 0x1FFF) {
        // MAIN MEMORY (Mirrored every 0x0800 bytes)
        return bus->main_memory[address & 0x07FF];

    } else if (address >= 0x2000 && address <= 0x3FFF) {
        // PPU Registers (Mirrored every 8 bytes)
        return cpu_ppu_read(bus->ppu, address & 0x0007, false);

    } else if (address >= 0x4016 && address <= 0x4017) {
        // Controller(s)
        data = (bus->controller_state[address & 0x0001] & 0x80) > 0;
        bus->controller_state[address & 0x0001] <<= 1;

    } else if (address >= 0x4020 && address <= 0x5FFF) {
        // Expansion ROM (Not implemented)
        printf("[BUS] Expansion ROM not implemented.\n");

    } else {
        printf("[BUS] Detected attempted read from undefined address on bus: 0x%04X\n", address);
    }

    // Return any fetched data, whether it be important or not, if not already returned.
    return data;
}
