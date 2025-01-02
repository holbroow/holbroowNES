#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct Mapper {
    uint8_t PRGbanks;
    uint8_t CHRbanks;

    bool (*MapperCpuRead)(struct Mapper *this, uint16_t address, uint32_t* mappedAddr);
    bool (*MapperCpuWrite)(struct Mapper *this, uint16_t address, uint32_t* mappedAddr);

    bool (*MapperPpuRead)(struct Mapper *this, uint16_t address, uint32_t* mappedAddr);
    bool (*MapperPpuWrite)(struct Mapper *this, uint16_t address, uint32_t* mappedAddr);
} Mapper;

Mapper *MapperCreate(uint8_t PRGbanks, uint8_t CHRbanks);

void MapperLoadNROM(Mapper *mapper);
