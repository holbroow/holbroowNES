#include "Cartridge.h"
#include "Mapper.h"

typedef struct iNesHeader {
    char asciiNes[4];
    uint8_t PRG_chunks;
    uint8_t CHR_chunks;
    uint8_t mapper1;
    uint8_t mapper2;
    uint8_t PRG_ramSize;
    uint8_t tvSystem1;
    uint8_t tvSystem2;
    char unusedPadding[5];
} iNesHeader;


Vector *VectorCreate(size_t initialCapacity) {
    Vector *vector = (Vector*)malloc(sizeof(Vector));
    if (!vector) { 
        fprintf(stderr, "Could't allocate space for the vector\n");
        exit(1);
    }
    vector->items = (uint8_t*)malloc(initialCapacity*sizeof(uint8_t));
    if (!vector->items) {
        fprintf(stderr, "Could't allocate space for the items of the vector\n");
        exit(1);
    }
    vector->size = 0;
    vector->capacity = initialCapacity;
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

    // If we find training information, ignore it as we dont need it :0)
    if (header.mapper1 & 0x04)
        fseek(file, 512, SEEK_CUR);

    // Now we read in the good stuff... the main cartridge contents
    Cartridge *cartridge = (Cartridge*)malloc(sizeof(Cartridge));
    if (!cartridge) { 
        fprintf(stderr, "[CARTRIDGE] Couldn't allocate space for the cartridge\n");
        exit(1);
    }
    // Creaate space for relevant cartridge data
    cartridge->PRGMemory = VectorCreate(16384*header.PRG_chunks);
    cartridge->CHRMemory = VectorCreate(8192*header.CHR_chunks);
    cartridge->nPRGBanks = header.PRG_chunks;
    cartridge->nCHRBanks = header.CHR_chunks;
    cartridge->mapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
    cartridge->mirror = (header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;

    // Store said cartridge data from file, and print for debugging purposes
    printf("[CARTRIDGE] PRG_chunks = %d\n", header.PRG_chunks);
    printf("[CARTRIDGE] CHR_chunks = %d\n", header.CHR_chunks);
    bytes = fread(cartridge->PRGMemory->items, sizeof(uint8_t), 16384*header.PRG_chunks, file);
    printf("[CARTRIDGE] PRGMemory read = %d\n", bytes);
    bytes = fread(cartridge->CHRMemory->items, sizeof(uint8_t), 8192*header.CHR_chunks, file);
    printf("[CARTRIDGE] CHRMemory read = %d\n", bytes);

    // Close file as it is no longer needed
    if (fclose(file) != 0) {
        fprintf(stdout, "[CARTRIDGE] Error closing the file\n");
        exit(1);
    }

    // Create mapper and load ROM into it depending on its Mapper ID
    // TODO: Implement further mapper IDs
    cartridge->mapper = MapperCreate(cartridge->nPRGBanks, cartridge->nCHRBanks);
    switch (cartridge->mapperID) {
        case 0:
            MapperLoadNROM(cartridge->mapper);
            break;
        default: printf("[CARTRIDGE] Mapper ID not implemented"); 
            break;
    }
    
    // Return the completed cartridge!
    return cartridge;
}

bool cartridge_cpu_read(Cartridge *cartridge, uint16_t addr, uint8_t* data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->MapperCpuRead(mapper, addr, &mapped_address)) {
        *data = cartridge->PRGMemory->items[mapped_address];
        return true;
    }

    return false;
}

bool cartridge_cpu_write(Cartridge *cartridge, uint16_t addr, uint8_t data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->MapperCpuWrite(mapper, addr, &mapped_address)) {
        cartridge->PRGMemory->items[mapped_address] = data;
        return true;
    }

    return false;
}

bool cartridge_ppu_read(Cartridge *cartridge, uint16_t addr, uint8_t* data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->MapperPpuRead(mapper, addr, &mapped_address)) {
        *data = cartridge->CHRMemory->items[mapped_address];
        return true;
    }

    return false;
}

bool cartridge_ppu_write(Cartridge *cartridge, uint16_t addr, uint8_t data) {
    uint32_t mapped_address = 0;
    Mapper *mapper = cartridge->mapper;

    if (mapper->MapperPpuWrite(mapper, addr, &mapped_address)) {
        cartridge->CHRMemory->items[mapped_address] = data;
        return true;
    }

    return false;
}
