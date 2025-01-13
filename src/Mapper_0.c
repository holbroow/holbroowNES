// Mapper_0.c
// Nintendo Entertainment System Mapper 0 (NROM) Implementation
// Will Holbrook - 3rd January 2025
#include <stdint.h>

#include "Mapper.h"

bool mapper_cpu_read(Mapper *mapper, uint16_t address, uint32_t *mapped_addr);
bool mapper_cpu_write(Mapper *mapper, uint16_t address, uint32_t *mapped_addr);

bool mapper_ppu_read(Mapper *mapper, uint16_t address, uint32_t *mapped_addr);
bool mapper_ppu_write(Mapper *mapper, uint16_t address, uint32_t *mapped_addr);

// Init Mapper 0
void mapper_load_0(Mapper *mapper) {
    mapper->mapper_cpu_read = mapper_cpu_read;
    mapper->mapper_cpu_write = mapper_cpu_write;
    mapper->mapper_ppu_read = mapper_ppu_read;
    mapper->mapper_ppu_write = mapper_ppu_write;
}

bool mapper_cpu_read(Mapper *mapper, uint16_t address, uint32_t *mapped_addr) {
    if (address >= 0x8000 && address <= 0xFFFF) {
        *mapped_addr = address & (mapper->prg_banks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }

    return false;
}

bool mapper_cpu_write(Mapper *mapper, uint16_t address, uint32_t *mapped_addr) {
    if (address >= 0x8000 && address <= 0xFFFF) {
        *mapped_addr = address & (mapper->prg_banks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }

    return false;
}

bool mapper_ppu_read(Mapper *mapper, uint16_t address, uint32_t *mapped_addr) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        *mapped_addr = address;
        return true;
    }

    return false;
}

bool mapper_ppu_write(Mapper *mapper, uint16_t address, uint32_t *mapped_addr) {
    if (address >= 0x0000 && address <= 0x1FFF) {
		if (mapper->chr_banks == 0) {
			*mapped_addr = address;
			return true;
		}
	}

    return false;
}
