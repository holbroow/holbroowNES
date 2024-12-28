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

// Define display parameters
#define NES_WIDTH 256
#define NES_HEIGHT 240
#define SCALE 2  // Scale factor for visibility

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

// Load the program to main bus memory
int load_program(Bus* bus, uint16_t start_address, const char* hex_string) {
    uint16_t current_address = start_address;   // Set the current memory offset to the intended start pos
    const char* ptr = hex_string;               // Set a pointer to the passed string containing the object code
    char buffer[3] = {0};                       // Two characters for a HEX byte
    int byte_index = 0;                         // Allow indexing through bytes

    while (*ptr != '\0') {  // While not at the end of the object code HEX string
        // Skip any spaces or newlines, should there be any
        if (isspace((unsigned char)*ptr)) {
            ptr++;
            continue;
        }

        // Read two hex characters to form a byte
        buffer[0] = toupper((unsigned char)*ptr);   // First character as first 4 bits
        ptr++;
        if (*ptr == '\0') {
            fprintf(stderr, "[MANAGER] Error, Incomplete byte in hex string while loading program bytes.\n");
            break;
        }
        buffer[1] = toupper((unsigned char)*ptr);   // Second character as second 4 bits to form the full byte
        ptr++;
        buffer[2] = '\0'; // Null-terminate the string

        // Convert the hex string to a byte
        char* endptr;
        uint8_t byte = (uint8_t)strtol(buffer, &endptr, 16);
        if (*endptr != '\0') {
            fprintf(stderr, "[MANAGER] Error, Invalid hex byte '%s' while loading program bytes.\n", buffer);
            continue; // Skip invalid byte
        }

        // Write the byte to memory
        bus_write(bus, current_address, byte);
        current_address++;

        // Check for memory bounds
        if (current_address > 0xFFFF) {
            fprintf(stderr, "[MANAGER] Error, Reached end of memory while loading program.\n");
            break;
        }
    }

    printf("[MANAGER] Program loaded successfully, starting at 0x%04X.\n", start_address);
    return start_address;
}



// Main function
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "[MANAGER] Usage: %s <path_to_nes_file>\n", argv[0]);
        return 1;
    }
    const char* nes_path = argv[1];


    // Initialise .nes game ('Cartridge')
    Cartridge* cart = init_cart(nes_path);

    // Initialize Bus
    printf("[MANAGER] Initialising BUS...\n");
    Bus* bus = init_bus();
    printf("[MANAGER] Initialising BUS finished!\n");

    // Initialize PPU
    printf("[MANAGER] Initialising PPU...\n");
    Ppu* ppu = init_ppu();
    printf("[MANAGER] Assigning PPU reference to the BUS...\n");
    bus->ppu = ppu;
    printf("[MANAGER] Assigning Game Cartridge reference to the BUS...\n");
    bus->cart = cart;
    printf("[MANAGER] Initialising PPU finished!\n");

    // Initialize CPU
    printf("[MANAGER] Initialising CPU...\n");
    Cpu* cpu = init_cpu(bus);
    printf("[MANAGER] Initialising CPU finished!\n");

    // Set program counter to the reset vector (0xFFFC-0xFFFD)
    uint16_t reset_low = bus_read(bus, 0xFFFC);
    uint16_t reset_high = bus_read(bus, 0xFFFD);
    uint16_t reset_vector = (reset_high << 8) | reset_low;
    cpu->PC = reset_vector;
    printf("[MANAGER] CPU PC set to reset vector 0x%04X\n", reset_vector);


    // Debug to check if cart->prg_rom (and therefore redundantly ppu->prg_rom) is of substance
    printf("\n[MANAGER] Program loaded:\n");
    for (size_t i = 0; i < cart->prg_rom_size; i++) {
        printf("%02X ", cart->prg_rom[i]);
        
        // Print 16 bytes per line
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    


    bool run_debug = false;
    bool run_capped = false;
    int capped_value = 50;

    // Run the NES!!!
    int frame_num = 0;
    int cycle_num = 0;

    cpu->running = true;
    printf("[CPU] CPU is now running!\n");
    if (run_debug) {
        printf("[CPU] Press ENTER to step through instructions.\n");
    }
    printf("\n");
    printf("[CPU] Current CPU State:\n");
    print_cpu(cpu);


    // Initialize frame timing
    uint64_t frame_start_time_ns = get_time_ns();
    cpu->cycles_left = CYCLES_PER_FRAME;
    
    while (cpu->running) {
        if (cpu->cycles_left > 0) {
            cycle_num++;

            if (run_debug) {
                // Debug: Cycles left before next frame
                printf("\n");
                printf("[CPU] CPU Cycles left: %d\n", cpu->cycles_left);
            }
            
            // Optionally cap the program at a number of instructions 
            if (run_capped && cpu->A == capped_value) {
                exit(0);
            }
            
            // Allow instruction stepping using the ENTER key
            if (run_debug) {
                int c  = getchar();
                while (c != '\n' && c != EOF) {
                    c = getchar();
                }
            }

            cpu_clock(cpu, run_debug, frame_num);

            // Run 3 PPU Cycles for every 1 CPU cycle
            for (size_t i = 1; i <= 3; i++) {
                ppu_clock(ppu);
            }
            
        } else {
            // Update Display (need to add)
            

            // Frame handling
            frame_num++;

            // Calculate how long the frame took
            uint64_t frame_end_time_ns = get_time_ns();
            uint64_t elapsed_time_ns = frame_end_time_ns - frame_start_time_ns;
            int64_t sleep_time_ns = FRAME_TIME_PERSEC - elapsed_time_ns;

            // Sleep for the remaining time to maintain 60fps
            if (sleep_time_ns > 0) {
                //if ((frame_num % 60) == 0) {
                    printf("[CPU] FRAME %d | Slept for %lld microseconds to retain clock speed.\n", frame_num, sleep_time_ns / 1000);
                //}
                sleep_microseconds((sleep_time_ns / 1000) - 1750);  // For some reason my timing is broken, and sleeps for slightly too long, this is jank... :0/
                // printf("%d\n", (sleep_time_ns / 1000));  // debug for time slept
            } else {
                // Frame took longer than desired
                //if ((frame_num % 60) == 0) {
                    printf("[CPU] FRAME %d | Warning, Frame is lagging by %lld ns.\n", frame_num, -sleep_time_ns);
                //}
            }

            // Start timer for the next frame
            frame_start_time_ns = get_time_ns();

            // Reset cycles_left to cycles per frame
            cpu->cycles_left = CYCLES_PER_FRAME;
        }
    }
        
    

    // Cleanup
    free(cpu);
    free(ppu);
    free(bus);

    return 0;
}