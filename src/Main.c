// Will Holbrook | Lancaster University Third Year Project 2024 (SCC 300: EmuPC)
// Main.c (TODO: Expl here!)

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
#define NES_WIDTH 256   // Pixel 'Width' of NES res
#define NES_HEIGHT 240  // Pixel 'Height' of NES res
#define SCALE 3         // Scale factor for improved visibility

const char* file_path;
Cartridge* cart;
Bus* bus;
Ppu* ppu;
Cpu* cpu;
SDL_Texture* texture;
SDL_Renderer* renderer;
int nes_cycles_passed = 0;
bool cpu_running;
bool run_debug = false;
int frame_num = 0;


// Initialise the NES as aa system (peripherals)
void init_nes() {
    // Initialise .nes game ('Cartridge')
    cart = init_cart(file_path);

    // Initialize Bus
    printf("[MANAGER] Initialising BUS...\n");
    bus = init_bus();
    printf("[MANAGER] Assigning Game Cartridge reference to the BUS...\n");
    bus->cart = cart;
    printf("[MANAGER] Assigning Controller 0 (Keyboard) to the BUS...\n");
    bus->controller[0] = 0x00;
    printf("[MANAGER] Initialising BUS finished!\n");

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
    renderer = SDL_CreateRenderer(SDL_CreateWindow("holbroowNES test", 50, 50, NES_WIDTH * SCALE, NES_HEIGHT * SCALE, SDL_WINDOW_SHOWN),
                                         -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING, NES_WIDTH, NES_HEIGHT);

    bool run_debug = false;
}

// One NES 'clock'
void nes_clock() {
    // Do one PPU 'clock'
        ppu_clock(ppu);

        // Run 1 cpu 'clock' for every 3 PPU cycles
        if (nes_cycles_passed % 3 == 0) {
            if (bus->dma_transfer) {
                if (bus->dma_dummy) {
                    if (nes_cycles_passed % 2 == 0) {
                        bus->dma_dummy = false;
                    }
                } else {
                    // DMA can take place!
                    if (nes_cycles_passed % 2 == 0)
                    {
                        // On even clock cycles, read from CPU bus
                        bus->dma_data = bus_read(bus, bus->dma_page << 8 | bus->dma_addr);
                    }
                    else
                    {
                        // On odd clock cycles, write to PPU OAM
                        bus->ppu->pOAM[bus->dma_addr] = bus->dma_data;
                        // Increment the lo byte of the address
                        bus->dma_addr++;
                        // If this wraps around, we know that 256
                        // bytes have been written, so end the DMA
                        // transfer, and proceed as normal
                        if (bus->dma_addr == 0x00)
                        {
                            bus->dma_transfer = false;
                            bus->dma_dummy = true;
                        }
                    }
                }
            } else {
                cpu_clock(cpu, run_debug, frame_num);
            }
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
    // printf("[CPU] CPU is now running!\n");
    // printf("\n");
    // printf("[CPU] Current CPU State:\n");
    // print_cpu(cpu);

    const uint32_t frame_duration_ms = 16;          // Store our target of ~60 FPS (16ms per frame)
    uint32_t frame_start_time_ms = SDL_GetTicks();  // Store the start time for initial frame

    // Run the NES!
    while (cpu->running) {
        // Handle any key presses (controller activity)
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        bus->controller[0] = 0x00;
        if (state[SDL_SCANCODE_Z])          bus->controller[0] |= 0x80;    // A        (Key Z)
        if (state[SDL_SCANCODE_X])          bus->controller[0] |= 0x40;    // B        (Key X)
        if (state[SDL_SCANCODE_TAB])        bus->controller[0] |= 0x20;    // Select   (Key SELECT)
        if (state[SDL_SCANCODE_RETURN])     bus->controller[0] |= 0x10;    // Start    (Key ENTER/RETURN)
        if (state[SDL_SCANCODE_UP])         bus->controller[0] |= 0x08;    // Up       (Key UP ARR)
        if (state[SDL_SCANCODE_DOWN])       bus->controller[0] |= 0x04;    // Down     (Key DOWN ARR)
        if (state[SDL_SCANCODE_LEFT])       bus->controller[0] |= 0x02;    // Left     (Key LEFT ARR)
        if (state[SDL_SCANCODE_RIGHT])      bus->controller[0] |= 0x01;    // Right    (Key RIGHT ARR)

        // Do 1 NES 'clock'
        nes_clock();

        // If the PPU completes the frame rendering process...
        if (ppu->frame_done) {
            // Reset flag
            ppu->frame_done = false;

            // Render frame to the SDL window/'display'
            SDL_UpdateTexture(texture, NULL, ppu->framebuffer, NES_WIDTH * sizeof(uint32_t));
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            // Increment the count (debug purposes only, otherwise serves no functional purpose)
            frame_num++;

            // Handle any SDL events
            for (SDL_Event event; SDL_PollEvent(&event);) {
                if (event.type == SDL_QUIT) {
                    return 0;
                }
            }

            // Attempt to time the frame to achieve ~60fps
            uint32_t frame_end_time_ms = SDL_GetTicks();
            int delay_time = frame_duration_ms - (frame_end_time_ms - frame_start_time_ms);
            if (delay_time > 0) {
                SDL_Delay(delay_time);
            }

            // Debug to check frequency of 60 frame update events
            if (frame_num % 60 == 0) {
                printf("60 frames passed/updated!\n");
            }

            // Update the start time for the next frame
            frame_start_time_ms = SDL_GetTicks(); // Reset frame timer
        }

    }

    // Clean up - Free NES components from memory
    free(cpu);
    free(ppu);
    free(bus);
    free(cart);

    // Bye bye!
    return 0;
}
