// Will Holbrook | Lancaster University Third Year Project 2024 (SCC 300: EmuPC)
// Main.c (Loads a (currently hard coded) program into memory and calls the 6502 CPU to 
// run from that address, hence running the stored program.)

#define SDL_MAIN_HANDLED

#include "Bus.h"
#include "holbroow6502.h"
#include "PPU.h"
#include "Cartridge.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <SDL2/SDL.h>

// Define display parameters
#define NES_WIDTH 256
#define NES_HEIGHT 240
#define SCALE 2  // Scale factor for visibility

const char* file_path;
Cartridge* cart;
Bus* bus;
Ppu* ppu;
Cpu* cpu;
void* texture;
void* renderer;
int nes_cycles_passed = 0;
bool cpu_running;
bool run_debug = false;
int frame_num = 0;


// Helper function to get current time in nanoseconds
uint64_t get_time_ns() {
    // ASSUMING WINDOWS (NTS: WOULD LIKE TO ADD COMPILER SUPPORT FOR LINUX LATER)
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (!QueryPerformanceFrequency(&frequency)) {
        fprintf(stderr, "[CPU] Time, QueryPerformanceFrequency failed!\n");
        return 0;
    }
    if (!QueryPerformanceCounter(&counter)) {
        fprintf(stderr, "[CPU] Time, QueryPerformanceCounter failed!\n");
        return 0;
    }
    return (uint64_t)((counter.QuadPart * 1000000000LL) / frequency.QuadPart);
}

// Cross-platform sleep function in microseconds
void sleep_microseconds(uint64_t microseconds) {
    // AGAIN, ASSUMING WINDOWS (NTS: WOULD LIKE TO ADD COMPILER SUPPORT FOR LINUX LATER)
    if (microseconds >= 1000) {
        // Sleep for the integer part of milliseconds
        DWORD millis = (DWORD)(microseconds / 1000);
        Sleep(millis);
    }

    // Busy-wait for the remaining microseconds
    LARGE_INTEGER frequency;
    LARGE_INTEGER start, current;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    double wait_time = (double)(microseconds % 1000) / 1000000.0; // Convert to seconds

    do {
        QueryPerformanceCounter(&current);
    } while (((double)(current.QuadPart - start.QuadPart) / frequency.QuadPart) < wait_time);
}

// Initialise the NES as aa system (peripherals)
void init_nes() {
    // Initialise .nes game ('Cartridge')
    cart = init_cart(file_path);

    // Initialize Bus
    printf("[MANAGER] Initialising BUS...\n");
    bus = init_bus();
    printf("[MANAGER] Initialising BUS finished!\n");
    printf("[MANAGER] Assigning Game Cartridge reference to the BUS...\n");
    bus->cart = cart;
    printf("[MANAGER] Assigned Game Cartridge reference to the BUS!\n");

    // Initialize PPU
    printf("[MANAGER] Initialising PPU...\n");
    ppu = init_ppu();
    printf("[MANAGER] Assigning PPU reference to the BUS...\n");
    bus->ppu = ppu;
    printf("[MANAGER] Assigning Game Cartridge reference to the PPU...\n");
    ppu->cart = cart;
    printf("[MANAGER] Initialising PPU finished!\n");

    // Initialize CPU
    printf("[MANAGER] Initialising CPU...\n");
    cpu = init_cpu(bus);
    printf("[MANAGER] Initialising CPU finished!\n");

    // Set program counter to the reset vector (0xFFFC-0xFFFD)
    uint16_t reset_low = bus_read(bus, 0xFFFC);
    uint16_t reset_high = bus_read(bus, 0xFFFD);
    uint16_t reset_vector = (reset_high << 8) | reset_low;
    cpu->PC = reset_vector;   // Normally set to reset_vector value unless modified for a test case etc...
    printf("[MANAGER] CPU PC set to reset vector 0x%04X\n", cpu->PC);
}

// Initialise the SDL2-based display
void init_display() {
    // Initialise Display
    SDL_Init(SDL_INIT_VIDEO);
    renderer = SDL_CreateRenderer(SDL_CreateWindow("holbroowNES test", 50, 50, 1024, 840, SDL_WINDOW_SHOWN),
                                         -1, SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                        SDL_TEXTUREACCESS_STREAMING, 256, 224);

    bool run_debug = false;
}

// One NES 'clock'
void nes_clock() {
    // Do one PPU 'clock'
        ppu_clock(ppu);

        // Run 1 cpu 'clock' for every 3 PPU cycles
        if (nes_cycles_passed % 3 == 0) {
            cpu_clock(cpu, run_debug, frame_num);
        }

        if (bus->ppu->nmi_occurred) {
            bus->ppu->nmi_occurred = false;
            cpu_nmi(cpu, bus);
        }

        nes_cycles_passed++;
}

// Main function
int main(int argc, char* argv[]) {
    // Take in filename/filepath to .nes game/program (local file)
    if (argc < 2) {
        fprintf(stderr, "[MANAGER] Usage: %s <path_to_nes_file>\n", argv[0]);
        return 1;
    }
    file_path = argv[1];


    // Initialise the 'NES' with a Cartridge, Bus, PPU, and CPU
    init_nes();


    // Debug to check if cart->prg_rom (and therefore redundantly ppu->cart->prg_rom) is of substance
    printf("\n[MANAGER] Program on cartridge:\n");
    for (size_t i = 0; i < cart->PRGMemory->capacity; i++) {
        printf("%02X ", cart->PRGMemory->items[i]);
        
        // Print 16 bytes per line
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");


    // Initialise Display
    init_display();


    // Run the NES!!!
    cpu->running = true;
    printf("[CPU] CPU is now running!\n");
    printf("\n");
    printf("[CPU] Current CPU State:\n");
    print_cpu(cpu);

    // Run the NES!
    while (cpu->running) {

        // Do 1 NES 'clock'
        nes_clock();


        // After a number of cycles (maybe cycles??? not sure...) update the display
        if (cpu->cycle_count % 29780 == 0) { // TODO: Need to figure this value out -  need for timing + when to update display???
            // Update Display
            SDL_UpdateTexture(texture, 0, ppu->framebuffer, NES_WIDTH * 4); // We multiply the width (256) by 4 to signify 4 bytes per pixel (uint32_t) for the pitch
            // SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, 0, 0);
            SDL_RenderPresent(renderer);

            // Handle SDL events
            for (SDL_Event event; SDL_PollEvent(&event);)
                if (event.type == SDL_QUIT)
                return 0;
        }

    }


    // Clean up
    free(cpu);
    free(ppu);
    free(bus);
    free(cart);

    return 0;
}
