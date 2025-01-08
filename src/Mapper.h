#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct Mapper {
    uint8_t prg_banks;
    uint8_t chr_banks;

    bool (*mapper_cpu_read)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);
    bool (*mapper_cpu_write)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);

    bool (*mapper_ppu_read)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);
    bool (*mapper_ppu_write)(struct Mapper *this, uint16_t address, uint32_t *mapped_addr);
} Mapper;

Mapper *mapper_create(uint8_t prg_banks, uint8_t chr_banks);

void mapper_load(Mapper *mapper);
