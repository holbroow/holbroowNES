// Cartridge.c
// Nintendo Entertainment System Cartridge Implementation
// Will Holbrook - 20th December 2024

#include "Cartridge.h"
#include "Mapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PRG_CHUNK_SIZE 16384
#define CHR_CHUNK_SIZE 8192

// NES file header structure with alternate field names.
typedef struct {
    char magic[4];
    uint8_t prg_count;
    uint8_t chr_count;
    uint8_t flag6;  // Contains mapper lower nibble, trainer flag, and mirroring info.
    uint8_t flag7;  // Contains mapper upper nibble.
    uint8_t prg_ram;
    uint8_t flag9;
    uint8_t flag10;
    char reserved[5];
} NESHeader;

// Helper to print an error message and exit.
static void exitWithError(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

// Helper to allocate a vector of the given capacity.
static Vector* createVector(size_t capacity) {
    Vector* vec = malloc(sizeof(Vector));
    if (!vec) {
        exitWithError("Failed to allocate Vector structure");
    }
    vec->items = malloc(capacity * sizeof(uint8_t));
    if (!vec->items) {
        free(vec);
        exitWithError("Failed to allocate memory for Vector items");
    }
    vec->size = 0;
    vec->capacity = capacity;
    return vec;
}

// Reads the NES header from the open file.
static NESHeader readHeader(FILE *file) {
    NESHeader hdr = {0};
    if (fread(&hdr, sizeof(NESHeader), 1, file) != 1) {
        exitWithError("Error reading NES header");
    }
    return hdr;
}

// Initializes a cartridge by loading its data from the specified file.
Cartridge* init_cart(const char* filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "[CARTRIDGE] Cannot open file '%s'\n", filepath);
        exit(EXIT_FAILURE);
    }

    NESHeader header = readHeader(fp);

    // If trainer is present (bit 2 in flag6), skip its 512 bytes.
    if (header.flag6 & 0x04) {
        if (fseek(fp, 512, SEEK_CUR) != 0) {
            fclose(fp);
            exitWithError("[CARTRIDGE] Failed to skip trainer data");
        }
    }

    Cartridge *cart = malloc(sizeof(Cartridge));
    if (!cart) {
        fclose(fp);
        exitWithError("[CARTRIDGE] Unable to allocate Cartridge structure");
    }

    // Set up cartridge bank counts and allocate memory for PRG and CHR.
    cart->n_prg_banks = header.prg_count;
    cart->n_chr_banks = header.chr_count;
    cart->prg_memory = createVector(PRG_CHUNK_SIZE * header.prg_count);
    cart->chr_memory = createVector(CHR_CHUNK_SIZE * header.chr_count);

    // Compute mapper ID and mirroring mode.
    cart->mapper_id = ((header.flag7 >> 4) << 4) | (header.flag6 >> 4);
    cart->mirror = (header.flag6 & 0x01) ? VERTICAL : HORIZONTAL;

    // Load PRG data.
    size_t prgTotal = PRG_CHUNK_SIZE * header.prg_count;
    size_t prgRead = fread(cart->prg_memory->items, sizeof(uint8_t), prgTotal, fp);
    // printf("[CARTRIDGE] PRG banks: %d, bytes read: %zu\n", header.prg_count, prgRead);

    // Load CHR data.
    size_t chrTotal = CHR_CHUNK_SIZE * header.chr_count;
    size_t chrRead = fread(cart->chr_memory->items, sizeof(uint8_t), chrTotal, fp);
    // printf("[CARTRIDGE] CHR banks: %d, bytes read: %zu\n", header.chr_count, chrRead);

    if (fclose(fp) != 0) {
        exitWithError("[CARTRIDGE] Error closing file");
    }

    // Create the mapper and load its configuration.
    cart->mapper = mapper_create(cart->n_prg_banks, cart->n_chr_banks,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    switch (cart->mapper_id) {
        case 0:
            mapper_load_0(cart->mapper);
            // printf("[CARTRIDGE] Mapper ID %d loaded (NROM)\n", cart->mapper_id);
            break;
        case 1:
            mapper_load_1(cart->mapper); // This mapper's implementation is currently dodgy and only partially functional for some games, although that is similarly the case for Mapper 0 (LOL!)
            // printf("[CARTRIDGE] Mapper ID %d loaded\n", cart->mapper_id);
            break;
        default:
            // printf("[CARTRIDGE] Mapper ID %d is not implemented\n", cart->mapper_id);
            break;
    }

    return cart;
}

// CPU read: translates the CPU address via the mapper and reads from PRG memory.
bool cartridge_cpu_read(Cartridge *cart, uint16_t addr, uint8_t *data) {
    uint32_t mappedAddr = 0;
    if (cart->mapper->mapper_cpu_read(cart->mapper, addr, &mappedAddr)) {
        *data = cart->prg_memory->items[mappedAddr];
        return true;
    }
    return false;
}

// CPU write: translates the CPU address via the mapper and writes to PRG memory.
bool cartridge_cpu_write(Cartridge *cart, uint16_t addr, uint8_t data) {
    uint32_t mappedAddr = 0;
    if (cart->mapper->mapper_cpu_write(cart->mapper, addr, &mappedAddr)) {
        cart->prg_memory->items[mappedAddr] = data;
        return true;
    }
    return false;
}

// PPU read: translates the PPU address via the mapper and reads from CHR memory.
bool cartridge_ppu_read(Cartridge *cart, uint16_t addr, uint8_t *data) {
    uint32_t mappedAddr = 0;
    if (cart->mapper->mapper_ppu_read(cart->mapper, addr, &mappedAddr)) {
        *data = cart->chr_memory->items[mappedAddr];
        return true;
    }
    return false;
}

// PPU write: translates the PPU address via the mapper and writes to CHR memory.
bool cartridge_ppu_write(Cartridge *cart, uint16_t addr, uint8_t data) {
    uint32_t mappedAddr = 0;
    if (cart->mapper->mapper_ppu_write(cart->mapper, addr, &mappedAddr)) {
        cart->chr_memory->items[mappedAddr] = data;
        return true;
    }
    return false;
}
