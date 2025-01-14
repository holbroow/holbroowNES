// Will Holbrook | NES Emulator in C | Lancaster University Third Year Project 2024 (SCC 300: EmuPC)

// Main.c
// Nintendo Entertainment System Implementation (This runs the NES!)
// Will Holbrook - Created 18th October 2024

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
#include <commdlg.h> // For GetOpenFileName
#include <SDL2/SDL.h>

// Define window params
// Menu IDs
#define ID_FILE_LOADROM 9003
#define ID_FILE_EXIT 9001
#define ID_HELP_HELP 9004
#define ID_HELP_ABOUT 9002

// Define display params
#define NES_WIDTH 256           // Screen pixel 'Width' of NES res
#define NES_HEIGHT (240 - 16)   // Screen pixel 'Height' of NES res (240 - 16 to retain 224 height (hidden CRT scanlines effect)) (we effectively cut off the top and bottom 8 scanlines)
#define SCALE 3                 // Scale factor for improved visibility

const char* file_path;

Cartridge* cart;
Bus* bus;
Ppu* ppu;
Cpu* cpu;

HWND hwnd;
MSG msg;
SDL_Window* sdl_window;
SDL_Texture* texture;
SDL_Renderer* renderer;

int nes_cycles_passed = 0;
bool nes_running;
bool cpu_running;
bool run_debug = false;
int frame_num = 0;
uint32_t frame_duration_ms;
uint32_t frame_start_time_ms;
uint32_t frame_end_time_ms;
int delay_time;

const uint8_t *state;


// Callback for handling Windows messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_FILE_LOADROM:
                {
                    // Buffer to store the file path
                    char filePath[MAX_PATH] = {0};

                    // Open file dialog
                    OPENFILENAME ofn = {0};
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = "ROM Files\0*.rom;*.bin;*.nes\0All Files\0*.*\0"; // Filter for ROM file types
                    ofn.lpstrFile = filePath;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    ofn.lpstrTitle = "Select a ROM File";

                    if (GetOpenFileName(&ofn)) {
                        // Free the previous file_path if it was already set
                        if (file_path) {
                            free((void*)file_path);  // Cast to void* to free a const char*
                        }

                        // Allocate memory for the new path and copy it
                        file_path = (char*)malloc(strlen(filePath) + 1);
                        if (file_path) {
                            strcpy((char*)file_path, filePath);
                            printf("Loaded ROM: %s\n", file_path);
                        } else {
                            MessageBox(hwnd, "Failed to allocate memory for file path.", "Error", MB_OK | MB_ICONERROR);
                        }
                    } else {
                        MessageBox(hwnd, "No file selected.", "Load ROM", MB_OK | MB_ICONWARNING);
                    }
                    nes_running = true;
                    break;
                }
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    break;

                case ID_HELP_HELP:
                    MessageBox(hwnd, "Help", "Help", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_HELP_ABOUT:
                    MessageBox(hwnd, "About", "About", MB_OK | MB_ICONINFORMATION);
                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

//Initialise 'holbroowNES' window
int init_window() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "SDLWindowClass";

    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        0, "SDLWindowClass", "holbroowNES",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        NES_WIDTH * SCALE, ((NES_HEIGHT * SCALE) + GetSystemMetrics(SM_CYMENU)), NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!hwnd) {
        MessageBox(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Create menu bar
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hHelpMenu = CreateMenu();

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_LOADROM, "Load ROM");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, "Quit");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");
    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_HELP, "Help");
    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, "About");

    SetMenu(hwnd, hMenu);
}

// Initialise the NES as a system (peripherals)
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
    printf("[MANAGER] CPU PC set to reset vector 0x%04X\n\n", cpu->PC);
}

// Initialise the SDL2-based display
void init_sdl_display() {
    // Initialise Display
    SDL_Init(SDL_INIT_VIDEO);
    // renderer = SDL_CreateRenderer(SDL_CreateWindow("holbroowNES test", 50, 50, NES_WIDTH * SCALE, NES_HEIGHT * SCALE, SDL_WINDOW_SHOWN),
    //                               -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    sdl_window = SDL_CreateWindowFrom((void*) hwnd);
    renderer = SDL_CreateRenderer(sdl_window,
                                  -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING, NES_WIDTH, (NES_HEIGHT));
    ShowWindow(hwnd, SW_SHOW);
    bool run_debug = false;
}

// Reset the NES system (Reset Button simulation)
void reset_nes(Cpu* cpu, Bus* bus, Ppu* ppu) {
    // Reset CPU
    cpu_reset(cpu, bus);
    cpu->cycle_count = 0;

    // Reset (not really) PPU
    memset(ppu->framebuffer, 0, sizeof(ppu->framebuffer));
    ppu->frames_completed = 0;
    
    // NES will now run from 'cycle' 0
    nes_cycles_passed = 0;
}

// One NES 'clock'
void nes_clock() {
    // Do one PPU 'clock'
    ppu_clock(ppu);

    // Run 1 CPU 'clock' for every 3 PPU cycles
    if (nes_cycles_passed % 3 == 0) {
        if (bus->dma_transfer) {
            if (bus->dma_dummy) {
                if (nes_cycles_passed % 2 == 0) {
                    bus->dma_dummy = false;
                }
            } else {
                // DMA can take place!
                if (nes_cycles_passed % 2 == 0) {
                    // On even clock cycles, read from CPU bus
                    bus->dma_data = bus_read(bus, bus->dma_page << 8 | bus->dma_addr);
                } else {
                    // On odd clock cycles, write to PPU OAM
                    bus->ppu->p_oam[bus->dma_addr] = bus->dma_data;
                    // Increment the low byte of the address
                    bus->dma_addr++;
                    // If this wraps around, we know that 256
                    // bytes have been written, so end the DMA
                    // transfer, and proceed as normal
                    if (bus->dma_addr == 0x00) {
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
    // if (argc < 2) {
    //     fprintf(stderr, "[MANAGER] Usage: %s <path_to_nes_file>\n", argv[0]);
    //     return 1;
    // }
    // file_path = argv[1];


    // Initialise 'holbroowNES' window (If error occurs, exit program)
    // if (init_window() == -1) return -1;
    init_window();

    // // Initialise the 'NES' with a Cartridge, Bus, PPU, and CPU
    // init_nes();

    // Initialise Display
    init_sdl_display();


    while (!nes_running) {
        // Handle Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                exit(0);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }

    printf("%s\n", file_path);
    // Initialise the 'NES' with a Cartridge, Bus, PPU, and CPU
    init_nes();

    // Frame / Keyboard Variables
    frame_duration_ms       = 16;                           // Store our target of ~60 FPS (16ms per frame)
    frame_start_time_ms     = SDL_GetTicks();               // Store the start time for initial frame
    state                   = SDL_GetKeyboardState(NULL);   // Configure a value to store keyboard's 'state' (what is/isn't pressed)

    while (nes_running) {
        // Handle Windows messages
        if (nes_cycles_passed % 100 == 0) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    nes_running = false;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // Handle any key presses (controller activity)
        bus->controller[0] = 0x00;                                          // Initiate Controller 0 (1st controller)

        if (state[SDL_SCANCODE_P])          nes_running = false;            // POWER    (Key P) This only really powers off :0)
        if (state[SDL_SCANCODE_R])          reset_nes(cpu, bus, ppu);       // RESET    (Key R)

        if (state[SDL_SCANCODE_Z])          bus->controller[0] |= 0x80;     // A        (Key Z)
        if (state[SDL_SCANCODE_X])          bus->controller[0] |= 0x40;     // B        (Key X)

        if (state[SDL_SCANCODE_TAB])        bus->controller[0] |= 0x20;     // Select   (Key SELECT)
        if (state[SDL_SCANCODE_RETURN])     bus->controller[0] |= 0x10;     // Start    (Key ENTER/RETURN)

        if (state[SDL_SCANCODE_UP])         bus->controller[0] |= 0x08;     // Up       (Key UP ARR)
        if (state[SDL_SCANCODE_DOWN])       bus->controller[0] |= 0x04;     // Down     (Key DOWN ARR)
        if (state[SDL_SCANCODE_LEFT])       bus->controller[0] |= 0x02;     // Left     (Key LEFT ARR)
        if (state[SDL_SCANCODE_RIGHT])      bus->controller[0] |= 0x01;     // Right    (Key RIGHT ARR)


        // Do 1 NES 'clock'
        /* Within 'nes_clock':
            Controller state is checked (KB input) (We pass in the Keyboard's state, as visible)
            The PPU is clocked thrice
            The CPU is clocked once
            DMA and NMI is partially handled here as well
        */
        nes_clock();


        // If the PPU completes the frame rendering process...
        if (ppu->frame_done) {
            // Reset flag
            ppu->frame_done = false;

            // Render frame to the SDL window/'display'
            SDL_UpdateTexture(texture, NULL, ppu->framebuffer + 2048, NES_WIDTH * sizeof(uint32_t)); // we skip 2048 bytes to skip the first 8 scanlines, and consequently the last 8 scanlines (224 height instead of 240)
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            frame_num++;    // Increment the count (debug purposes only, otherwise serves no functional purpose)

            // Handle any SDL events
            for (SDL_Event event; SDL_PollEvent(&event);) {
                if (event.type == SDL_QUIT) {
                    return 0;
                }
            }

            // Attempt to time the frame to achieve ~60fps
            frame_end_time_ms = SDL_GetTicks();
            int delay_time = frame_duration_ms - (int)(frame_end_time_ms - frame_start_time_ms);
            if (delay_time > 0) {
                SDL_Delay((uint32_t)delay_time);
            }

            // Debug to check frequency of 60 frame update events
            if (frame_num % 60 == 0) {
                printf("60 frames passed/updated!\n");
            }

            // Update the start time for the next frame
            frame_start_time_ms = SDL_GetTicks();   // Reset frame timer
        }
    }


    // Clean up - Free NES components + SDL/Window from memory
    if (file_path) {
        free((void*)file_path); // Free the allocated memory
    }
    free(cpu);
    free(ppu);
    free(bus);
    free(cart);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    // Bye bye!
    return 0;
}
