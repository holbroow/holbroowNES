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
        fprintf(stderr, "QueryPerformanceFrequency failed!\n");
        return 0;
    }
    if (!QueryPerformanceCounter(&counter)) {
        fprintf(stderr, "QueryPerformanceCounter failed!\n");
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
            fprintf(stderr, "MANAGER: Error, Incomplete byte in hex string while loading program bytes.\n");
            break;
        }
        buffer[1] = toupper((unsigned char)*ptr);   // Second character as second 4 bits to form the full byte
        ptr++;
        buffer[2] = '\0'; // Null-terminate the string

        // Convert the hex string to a byte
        char* endptr;
        uint8_t byte = (uint8_t)strtol(buffer, &endptr, 16);
        if (*endptr != '\0') {
            fprintf(stderr, "MANAGER: Error, Invalid hex byte '%s' while loading program bytes.\n", buffer);
            continue; // Skip invalid byte
        }

        // Write the byte to memory
        bus_write(bus, current_address, byte);
        current_address++;

        // Check for memory bounds
        if (current_address > 0xFFFF) {
            fprintf(stderr, "MANAGER: Error, Reached end of memory while loading program.\n");
            break;
        }
    }

    printf("MANAGER: Program loaded successfully, starting at 0x%04X.\n", start_address);
    return start_address;
}

// Main function
int main() {
    // Initialize Bus
    Bus* bus = init_bus();

    // Initialize CPU
    Cpu* cpu = init_cpu(bus);

    // Initialise PPU
    Ppu* ppu = init_ppu(bus);


    // Program code to load:    Multiply 10 by 3
    // Grab the start_address from the loaded program, to point the PC towards
    int start_address = load_program(bus, 0x0100, "A2 0A 8E 00 00 A2 03 8E 01 00 AC 00 00 A9 00 18 6D 01 00 88 D0 FA 8D 02 00 EA EA EA");

    // Set PC to start of program
    cpu->PC = start_address;


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
            
            // Here we can optionally cap the program at a number of instructions 
            // in order to save time while needing to test with large instruction sequences
            // without having to step 1 by 1.
            // Default = Initially disabled with an example value of 50
            if (run_capped && cpu->A == capped_value) {
                exit(0);
            }
            
            // Here we allow instruction stepping using the ENTER key
            if (run_capped) {
                int c  = getchar();
                while (c != '\n' && c != EOF) {
                    c = getchar();
                }
            }

            cpu_clock(cpu, run_debug, frame_num);

        } else {
            // NOTE: cycles_left is already decremented by the instruction handlers!
            // Calculate now long that prev 'frame' took (assuming not on instruction 0... if so, we skip this consideration)
            // Frame count is incremented for each frame after frame 0
            frame_num++;

            // Check how long below a second those 25,166 cycles took, sleep for rest until 1 sec
            // Stop timer
            uint64_t frame_end_time_ns = get_time_ns();
            uint64_t elapsed_time_ns = frame_end_time_ns - frame_start_time_ns;
            // Check difference between timer value and 1 second
            int64_t sleep_time_ns = FRAME_TIME_PERSEC - elapsed_time_ns;

            // Sleep for that (millisecond format) difference
            if (sleep_time_ns > 0) {
                if ((frame_num % 60) == 0) {
                    printf("CPU: Slept for %d microseconds to retain clock speed.\n", sleep_time_ns / 1000);
                }
                // Convert nanoseconds to microseconds for sleep_microseconds, and sleep
                sleep_microseconds(sleep_time_ns / 1000);
            } else {
                // Frame took longer than desired; consider logging or handling lag
                printf("CPU: Warning: Frame %d is lagging by %lld ns.\n", frame_num, -sleep_time_ns);
            }

            // Reset cycles_left to 28.166 (Cycles/frame based on 2A03 at 60Hz)
            cpu->cycles_left = CYCLES_PER_FRAME;
            
            // Start timer for frame-time
            frame_start_time_ns = get_time_ns();
        }

        // Run 3 PPU Cycles for every 1 cpu cycle
        for (size_t i = 1; i <= 3; i++) {
            //printf("PPU CYCLE %d/3 AT CPU CYCLE %d\n", i, cycle_num);
            
            ppu_clock(ppu);
        }
    }

    // Cleanup
    free(cpu);
    free(bus);
    // Might need to add any additional cleanup for the Bus if necessary, in the future...

    return 0;
}
