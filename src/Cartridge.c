// Cartridge.c
// Nintendo Entertainment System Cartridge Implementation
// Will Holbrook - 20th December 2024
#include "Cartridge.h"
#include "Mapper.h"

typedef struct iNesHeader {
    char ascii_nes[4];
    uint8_t prg_chunks;
    uint8_t chr_chunks;
    uint8_t mapper1;
    uint8_t mapper2;
    uint8_t prg_ram_size;
    uint8_t tv_system1;
    uint8_t tv_system2;
    char unused_padding[5];
} iNesHeader;


Vector *vector_create(size_t initial_capacity) {
    Vector *vector = (Vector*)malloc(sizeof(Vector));
    if (!vector) { 
        fprintf(stderr, "Couldn't allocate space for the vector\n");
        exit(1);
    }
    vector->items = (uint8_t*)malloc(initial_capacity * sizeof(uint8_t));
    if (!vector->items) {
        fprintf(stderr, "Couldn't allocate space for the items of the vector\n");
        exit(1);
    }
    vector->size = 0;
    vector->capacity = initial_capacity;
    return vector;
}


Cartridge* init_cart(const char* filepath) {
    // Attempt to open .nes file
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "[CARTRIDGE] Error opening '%s' file\n", filepath);
        exit(1);
    }

    // Create header storage and store the 'iNesHeader' from the file
    iNesHeader header = {0};
    size_t bytes = fread(&header, sizeof(iNesHeader), 1, file);

    // If we find trainer information, ignore it as we don't need it :)
    if (header.mapper1 & 0x04)
        fseek(file, 512, SEEK_CUR);

    // Now we read in the main cartridge contents
    Cartridge *cartridge = (Cartridge*)malloc(sizeof(Cartridge));
    if (!cartridge) { 
        fprintf(stderr, "[CARTRIDGE] Couldn't allocate space for the cartridge\n");
        exit(1);
    }
    // Create space for relevant cartridge data
    cartridge->prg_memory = vector_create(16384 * header.prg_chunks);
    cartridge->chr_memory = vector_create(8192 * header.chr_chunks);
    cartridge->n_prg_banks = header.prg_chunks;
    cartridge->n_chr_banks = header.chr_chunks;
    cartridge->mapper_id = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
    cartridge->mirror = (header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;

    // Store cartridge data from file, and print for debugging purposes
    printf("[CARTRIDGE] PRG_chunks = %d\n", header.prg_chunks);
    printf("[CARTRIDGE] CHR_chunks = %d\n", header.chr_chunks);
    bytes = fread(cartridge->prg_memory->items, sizeof(uint8_t), 16384 * header.prg_chunks, file);
    printf("[CARTRIDGE] PRGMemory read = %zu\n", bytes);
    bytes = fread(cartridge->chr_memory->items, sizeof(uint8_t), 8192 * header.chr_chunks, file);
    printf("[CARTRIDGE] CHRMemory read = %zu\n", bytes);

    // Close file as it is no longer needed
    if (fclose(file) != 0) {
        fprintf(stdout, "[CARTRIDGE] Error closing the file\n");
        exit(1);
    }

    // Create mapper and load ROM into it depending on its Mapper ID
    // TODO: Implement further mapper IDs
    switch (cartridge->mapper_id) {
        case 0:
            cartridge->mapper = mapper_create(cartridge->n_prg_banks, 
                                              cartridge->n_chr_banks, 
                                              NULL, 
                                              NULL, 
                                              NULL,
                                              NULL, 
                                              NULL, 
                                              NULL, 
                                              NULL);
            mapper_load_0(cartridge->mapper);
            printf("[CARTRIDGE] Mapper ID (%d) implemented\n", cartridge->mapper_id); 
            break;
        case 1:
            

        default:
            printf("[CARTRIDGE] Mapper ID (%d) not implemented\n", cartridge->mapper_id); 
            break;
    }
    
    // Return the completed cartridge!
    return cartridge;
}

bool cartridge_cpu_read(Cartridge *cartridge, uint16_t addr, uint8_t* data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->mapper_cpu_read(mapper, addr, &mapped_address)) {
        *data = cartridge->prg_memory->items[mapped_address];
        return true;
    }

    return false;
}

bool cartridge_cpu_write(Cartridge *cartridge, uint16_t addr, uint8_t data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->mapper_cpu_write(mapper, addr, &mapped_address)) {
        cartridge->prg_memory->items[mapped_address] = data;
        return true;
    }

    return false;
}

bool cartridge_ppu_read(Cartridge *cartridge, uint16_t addr, uint8_t* data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->mapper_ppu_read(mapper, addr, &mapped_address)) {
        *data = cartridge->chr_memory->items[mapped_address];
        return true;
    }

    return false;
}

bool cartridge_ppu_write(Cartridge *cartridge, uint16_t addr, uint8_t data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->mapper_ppu_write(mapper, addr, &mapped_address)) {
        cartridge->chr_memory->items[mapped_address] = data;
        return true;
    }

    return false;
}
