#include <stdint.h>

#include "Mapper.h"

bool MapperCpuRead(Mapper *mapper, uint16_t address, uint32_t* mappedAddr);
bool MapperCpuWrite(Mapper *mapper, uint16_t address, uint32_t* mappedAddr);

bool MapperPpuRead(Mapper *mapper, uint16_t address, uint32_t* mappedAddr);
bool MapperPpuWrite(Mapper *mapper, uint16_t address, uint32_t* mappedAddr);

void MapperLoadNROM(Mapper *mapper) {
    mapper->MapperCpuRead = MapperCpuRead;
    mapper->MapperCpuWrite = MapperCpuWrite;
    mapper->MapperPpuRead = MapperPpuRead;
    mapper->MapperPpuWrite = MapperPpuWrite;
}

bool MapperCpuRead(Mapper *mapper, uint16_t address, uint32_t* mappedAddr) {
    if (address >= 0x8000 && address <= 0xFFFF) {
        *mappedAddr = address & (mapper->PRGbanks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }

    return false;
}

bool MapperCpuWrite(Mapper *mapper, uint16_t address, uint32_t* mappedAddr) {
    if (address >= 0x8000 && address <= 0xFFFF) {
        *mappedAddr = address & (mapper->PRGbanks > 1 ? 0x7FFF : 0x3FFF);
        return true;
    }

    return false;
}

bool MapperPpuRead(Mapper *mapper, uint16_t address, uint32_t* mappedAddr) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        *mappedAddr = address;
        return true;
    }

    return false;
}

bool MapperPpuWrite(Mapper *mapper, uint16_t address, uint32_t* mappedAddr) {
    // Not implemented :0/

    return false;
}
