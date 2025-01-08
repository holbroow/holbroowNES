#include <stdint.h>

#include "mapper.h"

Mapper *mapper_create(uint8_t prg_banks, uint8_t chr_banks) {
    Mapper *new_mapper = (Mapper *)malloc(sizeof(Mapper));

    new_mapper->prg_banks = prg_banks;
    new_mapper->chr_banks = chr_banks;

    new_mapper->mapper_cpu_read = NULL;
    new_mapper->mapper_cpu_write = NULL;
    new_mapper->mapper_ppu_read = NULL;
    new_mapper->mapper_ppu_write = NULL;

    return new_mapper;
}
