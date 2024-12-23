// Will Holbrook | Lancaster University Third Year Project 2024 (SCC 300: EmuPC)
// Main.c (Loads a (currently hard coded) program into memory and calls the 6502 CPU to 
// run from that address, hence running the stored program.)

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

// Helper function to get current time in nanoseconds
uint64_t get_time_ns() {
    // ASSUMING WINDOWS (NTS: WOULD LIKE TO ADD COMPILER SUPPORT FOR LINUX LATER)
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (!QueryPerformanceFrequency(&frequency)) {
        fprintf(stderr, "CPU: Time, QueryPerformanceFrequency failed!\n");
        return 0;
    }
    if (!QueryPerformanceCounter(&counter)) {
        fprintf(stderr, "CPU: Time, QueryPerformanceCounter failed!\n");
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
        fprintf(stderr, "Usage: %s <path_to_nes_file>\n", argv[0]);
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
    printf("MANAGER: CPU PC set to reset vector 0x%04X\n", reset_vector);

    // // Program code to load:    Multiply 10 by 3
    // // Grab the start_address from the loaded program, to point the PC towards
    // int start_address = load_program(bus, 0x0100, "A2 0A 8E 00 00 A2 03 8E 01 00 AC 00 00 A9 00 18 6D 01 00 88 D0 FA 8D 02 00 EA EA EA");

    // // Set PC to start of program
    // cpu->PC = start_address;

    // Run the NES!!!
    int frame_num = 0;
    int cycle_num = 0;

    printf("CPU: CPU is now running!\n");
    printf("CPU: Press ENTER to step through instructions.\n");
    printf("\n");
    printf("Current CPU State:\n");
    print_cpu(cpu);

    /* Frequency Cycle Accurate Implementation:
        1. At the start of a frame (when cycles_left == 0), reset cycle count to cycles per frame as defined
        2. Run instructions indifinitely in a loop until cycles_left == 0
        3. Calculate how long that took to reach 0, and sleep for any remaining time to maintain 60fps (60Hz)
        4. Repeat
    */
    bool run_debug = false;
    bool run_capped = false;
    int capped_value = 50;

    // Initialize frame timing
    uint64_t frame_start_time_ns = get_time_ns();
    cpu->cycles_left = CYCLES_PER_FRAME;
    
    while (cpu->running) {
        if (cpu->cycles_left > 0) {
            cycle_num++;

            if (run_debug) {
                // Debug: Cycles left before next frame
                printf("\n");
                printf("CPU Cycles left: %d\n", cpu->cycles_left);
            }
            
            // Optionally cap the program at a number of instructions 
            if (run_capped && cpu->A == capped_value) {
                exit(0);
            }
            
            // Allow instruction stepping using the ENTER key
            if (run_capped) {
                int c  = getchar();
                while (c != '\n' && c != EOF) {
                    c = getchar();
                }
            }

            cpu_clock(cpu, run_debug, frame_num);
            
        } else {
            // Frame handling
            frame_num++;

            // Calculate how long the frame took
            uint64_t frame_end_time_ns = get_time_ns();
            uint64_t elapsed_time_ns = frame_end_time_ns - frame_start_time_ns;
            int64_t sleep_time_ns = FRAME_TIME_PERSEC - elapsed_time_ns;

            // Sleep for the remaining time to maintain 60fps
            if (sleep_time_ns > 0) {
                if ((frame_num % 60) == 0) {
                    printf("CPU: Slept for %lld microseconds to retain clock speed.\n", sleep_time_ns / 1000);
                }
                sleep_microseconds(sleep_time_ns / 1000);
            } else {
                // Frame took longer than desired
                printf("CPU: Warning: Frame %d is lagging by %lld ns.\n", frame_num, -sleep_time_ns);
            }

            // Reset cycles_left to cycles per frame
            cpu->cycles_left = CYCLES_PER_FRAME;
            
            // Start timer for the next frame
            frame_start_time_ns = get_time_ns();
        }

        // Run 3 PPU Cycles for every 1 CPU cycle
        for (size_t i = 1; i <= 3; i++) {
            ppu_clock(ppu);
        }

        
    }

    // Cleanup
    free(cpu);
    free(ppu);
    free(bus);

    return 0;
}