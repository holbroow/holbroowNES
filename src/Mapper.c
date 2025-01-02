#include <stdint.h>

#include "Mapper.h"

Mapper *MapperCreate(uint8_t PRGbanks, uint8_t CHRbanks) {
    Mapper *mapper = (Mapper*)malloc(sizeof(Mapper));

    mapper->PRGbanks = PRGbanks;
    mapper->CHRbanks = CHRbanks;

    mapper->MapperCpuRead = NULL;
    mapper->MapperCpuWrite = NULL;
    mapper->MapperPpuRead = NULL;
    mapper->MapperPpuWrite = NULL;

    return mapper;
}
