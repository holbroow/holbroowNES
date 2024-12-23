#include "Cartridge.h"

Cartridge* init_cart(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "[Cartridge] Failed to open file %s\n", filepath);
        exit(1);
    }
    printf("[Cartridge] Loading .nes file '%s'...\n", filepath);

    Cartridge* cart = malloc(sizeof(Cartridge));
    if (!cart) {
        fprintf(stderr, "[Cartridge] Memory allocation failed\n");
        fclose(file);
        exit(1);
    }

    // Read header (16 bytes)
    size_t bytes_read = fread(cart->magic, sizeof(char), 4, file);
    if (bytes_read != 4) {
        fprintf(stderr, "[Cartridge] Failed to read magic number\n");
        free(cart);
        fclose(file);
        exit(1);
    }

    // Verify magic number
    if (cart->magic[0] != 'N' || cart->magic[1] != 'E' || cart->magic[2] != 'S' || cart->magic[3] != 0x1A) {
        fprintf(stderr, "[Cartridge] Invalid magic number. Not a valid .nes file.\n");
        free(cart);
        fclose(file);
        exit(1);
    }

    // Read remaining header bytes (12 bytes)
    bytes_read = fread(&cart->prg_rom_banks, sizeof(uint8_t), 12, file);
    if (bytes_read != 12) {
        fprintf(stderr, "[Cartridge] Failed to read complete header\n");
        free(cart);
        fclose(file);
        exit(1);
    }

    // Parse flags
    cart->flags6 = cart->prg_rom_banks;
    cart->prg_rom_banks = cart->prg_rom_banks; // Already read
    cart->chr_rom_banks = cart->chr_rom_banks; // Already read
    cart->flags7 = cart->flags6; // Correction needed
    // The above lines need correction. Re-reading to parse correctly.

    // Correct parsing after reading all 16 bytes
    // Let's reset the reading sequence

    // Reset file pointer to start
    fseek(file, 0, SEEK_SET);

    uint8_t header[16];
    bytes_read = fread(header, sizeof(uint8_t), 16, file);
    if (bytes_read != 16) {
        fprintf(stderr, "[Cartridge] Failed to read complete header\n");
        free(cart);
        fclose(file);
        exit(1);
    }

    // Assign header fields
    cart->prg_rom_banks = header[4];
    cart->chr_rom_banks = header[5];
    cart->flags6 = header[6];
    cart->flags7 = header[7];
    cart->flags8 = header[8];
    cart->flags9 = header[9];
    cart->flags10 = header[10];
    for (int i = 11; i < 16; i++) {
        cart->padding[i - 11] = header[i];
    }

    // Parse mapper number
    cart->mapper = (header[7] & 0xF0) | ((header[6] & 0xF0) >> 4);

    // Parse flags
    cart->mirroring = (header[6] & 0x01) ? true : false;
    cart->battery = (header[6] & 0x02) ? true : false;
    cart->has_trainer = (header[6] & 0x04) ? true : false;
    cart->four_screen = (header[6] & 0x08) ? true : false;

    // NES 2.0 flag
    cart->nes2 = (header[7] & 0x0C) == 0x08 ? true : false;

    // Check for NES 2.0
    if (cart->nes2) {
        // Handle NES 2.0 specific parsing if necessary
        fprintf(stderr, "[Cartridge] NES 2.0 format not supported yet.\n");
        // Implement NES 2.0 parsing here
        free(cart->prg_rom);
        free(cart->chr_rom);
        free(cart);
        fclose(file);
        exit(1);
    }

    // Read Trainer if present
    if (cart->has_trainer) {
        bytes_read = fread(cart->trainer, sizeof(uint8_t), TRAINER_SIZE, file);
        if (bytes_read != TRAINER_SIZE) {
            fprintf(stderr, "[Cartridge] Failed to read trainer data\n");
            free(cart);
            fclose(file);
            exit(1);
        }
    }

    // Calculate PRG ROM size
    cart->prg_rom_size = cart->prg_rom_banks * 16 * 1024; // 16KB per bank
    if (cart->prg_rom_size > MAX_PRG_ROM_SIZE) {
        fprintf(stderr, "[Cartridge] PRG ROM size exceeds maximum supported size.\n");
        free(cart);
        fclose(file);
        exit(1);
    }

    // Allocate memory for PRG ROM
    cart->prg_rom = malloc(cart->prg_rom_size);
    if (!cart->prg_rom) {
        fprintf(stderr, "[Cartridge] Failed to allocate memory for PRG ROM\n");
        free(cart);
        fclose(file);
        exit(1);
    }

    // Read PRG ROM data
    bytes_read = fread(cart->prg_rom, sizeof(uint8_t), cart->prg_rom_size, file);
    if (bytes_read != cart->prg_rom_size) {
        fprintf(stderr, "[Cartridge] Failed to read PRG ROM data\n");
        free(cart->prg_rom);
        free(cart);
        fclose(file);
        exit(1);
    }

    // Calculate CHR ROM size
    cart->chr_rom_size = cart->chr_rom_banks * 8 * 1024; // 8KB per bank
    if (cart->chr_rom_size > MAX_CHR_ROM_SIZE) {
        fprintf(stderr, "[Cartridge] CHR ROM size exceeds maximum supported size.\n");
        free(cart->prg_rom);
        free(cart);
        fclose(file);
        exit(1);
    }

    // Allocate memory for CHR ROM
    if (cart->chr_rom_banks > 0) {
        cart->chr_rom = malloc(cart->chr_rom_size);
        if (!cart->chr_rom) {
            fprintf(stderr, "[Cartridge] Failed to allocate memory for CHR ROM\n");
            free(cart->prg_rom);
            free(cart);
            fclose(file);
            exit(1);
        }

        // Read CHR ROM data
        bytes_read = fread(cart->chr_rom, sizeof(uint8_t), cart->chr_rom_size, file);
        if (bytes_read != cart->chr_rom_size) {
            fprintf(stderr, "[Cartridge] Failed to read CHR ROM data\n");
            free(cart->chr_rom);
            free(cart->prg_rom);
            free(cart);
            fclose(file);
            exit(1);
        }
    } else {
        // Some cartridges use CHR RAM instead of CHR ROM
        cart->chr_rom = NULL;
    }

    fclose(file);

    printf("[Cartridge] Loaded %d PRG ROM banks (%zu KB)\n", cart->prg_rom_banks, cart->prg_rom_size / 1024);
    printf("[Cartridge] Loaded %d CHR ROM banks (%zu KB)\n", cart->chr_rom_banks, cart->chr_rom_size / 1024);
    printf("[Cartridge] Mapper #%d\n", cart->mapper);
    printf("[Cartridge] Mirroring: %s\n", cart->mirroring ? "Vertical" : "Horizontal");
    if (cart->has_trainer) {
        printf("[Cartridge] Trainer present\n");
    }
    if (cart->battery) {
        printf("[Cartridge] Battery-backed RAM present\n");
    }
    if (cart->four_screen) {
        printf("[Cartridge] Four-screen mirroring true\n");
    }

    printf("[Cartridge] Successfully loaded '%s'!\n", filepath);
    return cart;
}

void free_cart(Cartridge* cart) {
    if (!cart) return;
    if (cart->prg_rom) free(cart->prg_rom);
    if (cart->chr_rom) free(cart->chr_rom);
    free(cart);
}
