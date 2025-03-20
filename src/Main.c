// Will Holbrook | NES Emulator in C | Lancaster University Third Year Project 2024 (SCC 300: EmuPC)

// Main.c
// Nintendo Entertainment System Implementation (This runs the NES!)
// Will Holbrook - Created 18th October 2024

#define SDL_MAIN_HANDLED

#include "Bus.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>    // For the Windows API (window, menubar, etc.)
#include <commdlg.h>    // For GetOpenFileName
#include <SDL2/SDL.h>   // For the SDL-powered NES System display

// Define window params
// Menu IDs
#define ID_FILE_LOADROM 9001
#define ID_FILE_EXIT 9002
#define ID_HELP_HELP 9003
#define ID_HELP_ABOUT 9004
#define ID_CONTROL_RESET 9005
#define ID_CONTROL_POWER 9006
#define ID_CONFIG_CONTROLS 9007

// Define display params
#define NES_WIDTH   256             // Screen pixel 'Width' of NES res
#define NES_HEIGHT  (240 - 16)      // Screen pixel 'Height' of NES res (240 - 16 to retain 224 height (hidden CRT scanlines effect)) (we effectively cut off the top and bottom 8 scanlines)
#define SCALE       4               // Scale factor for improved visibility

// Define program window params
#define WINDOW_WIDTH (NES_WIDTH * SCALE)
#define WINDOW_HEIGHT ((NES_HEIGHT * SCALE) + GetSystemMetrics(SM_CYMENU))
#define WINDOW_X_POS 100
#define WINDOW_Y_POS 100

// 'Define' default key binds, these are to be changed by the user at runtime, if needed
SDL_Scancode key_power   = SDL_SCANCODE_P;
SDL_Scancode key_reset   = SDL_SCANCODE_R;
SDL_Scancode key_a       = SDL_SCANCODE_Z;
SDL_Scancode key_b       = SDL_SCANCODE_X;
SDL_Scancode key_select  = SDL_SCANCODE_TAB;
SDL_Scancode key_start   = SDL_SCANCODE_RETURN;
SDL_Scancode key_up      = SDL_SCANCODE_UP;
SDL_Scancode key_down    = SDL_SCANCODE_DOWN;
SDL_Scancode key_left    = SDL_SCANCODE_LEFT;
SDL_Scancode key_right   = SDL_SCANCODE_RIGHT;

uint32_t frame_duration_ms;
uint32_t frame_start_time_ms;
uint32_t frame_end_time_ms;
uint32_t frame_num = 0;

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
int frame_me_end_time_ms;
int delay_time;

const uint8_t *state;

void cleanup() {
    // Clean up - Free NES components + SDL/Window from memory
    if (file_path) {
        free((void*)file_path);
    }
    free(cpu);
    free(ppu);
    free(bus);
    free(cart);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(sdl_window);
    free(hwnd);
    SDL_Quit();
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

// Update SDL2-based display
void update_sdl_display() {
    SDL_UpdateTexture(texture, NULL, ppu->framebuffer + 2048, NES_WIDTH * sizeof(uint32_t)); // we skip 2048 bytes to skip the first 8 scanlines, and consequently the last 8 scanlines (224 height instead of 240)
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

// Reset the NES system (Reset Button simulation)
void reset_nes(Cpu* cpu, Bus* bus, Ppu* ppu) {
    // Reset CPU
    cpu_reset(cpu, bus);
    cpu->cycle_count = 0;

    // Reset (not really) PPU
    memset(ppu->framebuffer, 0, sizeof(ppu->framebuffer));
    ppu->frames_completed = 0;

    update_sdl_display();
    
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

// Load ROM
void load_rom() {
    // Track whether a new cartridge was selected, 
    //  so as to not reset the current nes program if nothing was selected
    bool cart_changed = false;

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
            cart_changed = true;
        } else {
            MessageBox(hwnd, "Failed to allocate memory for file path.", "Error", MB_OK | MB_ICONERROR);
        }
    } else {
        MessageBox(hwnd, "No file selected.", "Load ROM", MB_OK | MB_ICONWARNING);
    }

    // Check for NES' state and act accordingly, this is messy and there is 
    //  probably a much more efficient way to do these checks ;0)
    if (nes_running && cart_changed) {
        // Reset and assign cartridge
        cart = init_cart(file_path);
        bus->cart = cart;
        ppu->cart = cart;
        // Reset the NES
        reset_nes(cpu, bus, ppu);
    } else if ((nes_running && !cart_changed) || (!nes_running && cart_changed)) {
        nes_running = true;
    } else {
        nes_running = false;
    }
}


//// TO WHOEVER MAY READ THIS: Sorry this is so messy, this was a result of a few late nights learning how to create proper Windows API windows with buttons and drop downs.
////                           I am sure this could be a lot more elegant and cleaner, and my 'WindowProc' handler is stupidly messy with many functions not being abstracted to functon calls, but this was efficient
////                           at a point where I had little time and wanted to produce a good quality ( at least at the front end! :0) ) program. Please forgive me.

// Control Config Helper: Map a key name string to an SDL_Scancode
SDL_Scancode GetScancodeFromString(const char* keyName) {
    if (strcmp(keyName, "A") == 0) return SDL_SCANCODE_A;
    if (strcmp(keyName, "B") == 0) return SDL_SCANCODE_B;
    if (strcmp(keyName, "C") == 0) return SDL_SCANCODE_C;
    if (strcmp(keyName, "D") == 0) return SDL_SCANCODE_D;
    if (strcmp(keyName, "E") == 0) return SDL_SCANCODE_E;
    if (strcmp(keyName, "F") == 0) return SDL_SCANCODE_F;
    if (strcmp(keyName, "G") == 0) return SDL_SCANCODE_G;
    if (strcmp(keyName, "H") == 0) return SDL_SCANCODE_H;
    if (strcmp(keyName, "I") == 0) return SDL_SCANCODE_I;
    if (strcmp(keyName, "J") == 0) return SDL_SCANCODE_J;
    if (strcmp(keyName, "K") == 0) return SDL_SCANCODE_K;
    if (strcmp(keyName, "L") == 0) return SDL_SCANCODE_L;
    if (strcmp(keyName, "M") == 0) return SDL_SCANCODE_M;
    if (strcmp(keyName, "N") == 0) return SDL_SCANCODE_N;
    if (strcmp(keyName, "O") == 0) return SDL_SCANCODE_O;
    if (strcmp(keyName, "P") == 0) return SDL_SCANCODE_P;
    if (strcmp(keyName, "Q") == 0) return SDL_SCANCODE_Q;
    if (strcmp(keyName, "R") == 0) return SDL_SCANCODE_R;
    if (strcmp(keyName, "S") == 0) return SDL_SCANCODE_S;
    if (strcmp(keyName, "T") == 0) return SDL_SCANCODE_T;
    if (strcmp(keyName, "U") == 0) return SDL_SCANCODE_U;
    if (strcmp(keyName, "V") == 0) return SDL_SCANCODE_V;
    if (strcmp(keyName, "W") == 0) return SDL_SCANCODE_W;
    if (strcmp(keyName, "X") == 0) return SDL_SCANCODE_X;
    if (strcmp(keyName, "Y") == 0) return SDL_SCANCODE_Y;
    if (strcmp(keyName, "Z") == 0) return SDL_SCANCODE_Z;
    if (strcmp(keyName, "0") == 0) return SDL_SCANCODE_0;
    if (strcmp(keyName, "1") == 0) return SDL_SCANCODE_1;
    if (strcmp(keyName, "2") == 0) return SDL_SCANCODE_2;
    if (strcmp(keyName, "3") == 0) return SDL_SCANCODE_3;
    if (strcmp(keyName, "4") == 0) return SDL_SCANCODE_4;
    if (strcmp(keyName, "5") == 0) return SDL_SCANCODE_5;
    if (strcmp(keyName, "6") == 0) return SDL_SCANCODE_6;
    if (strcmp(keyName, "7") == 0) return SDL_SCANCODE_7;
    if (strcmp(keyName, "8") == 0) return SDL_SCANCODE_8;
    if (strcmp(keyName, "9") == 0) return SDL_SCANCODE_9;
    if (strcmp(keyName, "UP") == 0) return SDL_SCANCODE_UP;
    if (strcmp(keyName, "DOWN") == 0) return SDL_SCANCODE_DOWN;
    if (strcmp(keyName, "LEFT") == 0) return SDL_SCANCODE_LEFT;
    if (strcmp(keyName, "RIGHT") == 0) return SDL_SCANCODE_RIGHT;
    if (strcmp(keyName, "TAB") == 0) return SDL_SCANCODE_TAB;
    if (strcmp(keyName, "RETURN") == 0) return SDL_SCANCODE_RETURN;
    return SDL_SCANCODE_UNKNOWN;
}

// Control Config Helper: Convert an SDL_Scancode into its string representation
const char* ScancodeToString(SDL_Scancode scancode) {
    switch(scancode) {
        case SDL_SCANCODE_A: return "A";
        case SDL_SCANCODE_B: return "B";
        case SDL_SCANCODE_C: return "C";
        case SDL_SCANCODE_D: return "D";
        case SDL_SCANCODE_E: return "E";
        case SDL_SCANCODE_F: return "F";
        case SDL_SCANCODE_G: return "G";
        case SDL_SCANCODE_H: return "H";
        case SDL_SCANCODE_I: return "I";
        case SDL_SCANCODE_J: return "J";
        case SDL_SCANCODE_K: return "K";
        case SDL_SCANCODE_L: return "L";
        case SDL_SCANCODE_M: return "M";
        case SDL_SCANCODE_N: return "N";
        case SDL_SCANCODE_O: return "O";
        case SDL_SCANCODE_P: return "P";
        case SDL_SCANCODE_Q: return "Q";
        case SDL_SCANCODE_R: return "R";
        case SDL_SCANCODE_S: return "S";
        case SDL_SCANCODE_T: return "T";
        case SDL_SCANCODE_U: return "U";
        case SDL_SCANCODE_V: return "V";
        case SDL_SCANCODE_W: return "W";
        case SDL_SCANCODE_X: return "X";
        case SDL_SCANCODE_Y: return "Y";
        case SDL_SCANCODE_Z: return "Z";
        case SDL_SCANCODE_0: return "0";
        case SDL_SCANCODE_1: return "1";
        case SDL_SCANCODE_2: return "2";
        case SDL_SCANCODE_3: return "3";
        case SDL_SCANCODE_4: return "4";
        case SDL_SCANCODE_5: return "5";
        case SDL_SCANCODE_6: return "6";
        case SDL_SCANCODE_7: return "7";
        case SDL_SCANCODE_8: return "8";
        case SDL_SCANCODE_9: return "9";
        case SDL_SCANCODE_UP: return "UP";
        case SDL_SCANCODE_DOWN: return "DOWN";
        case SDL_SCANCODE_LEFT: return "LEFT";
        case SDL_SCANCODE_RIGHT: return "RIGHT";
        case SDL_SCANCODE_TAB: return "TAB";
        case SDL_SCANCODE_RETURN: return "RETURN";
        default: return "";
    }
}

// List of key names used to populate the control boxes.
const char* keyList[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
    "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
    "U", "V", "W", "X", "Y", "Z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "UP", "DOWN", "LEFT", "RIGHT", "TAB", "RETURN"
};
const int keyListCount = sizeof(keyList) / sizeof(keyList[0]);


// Config window procedure: creates labels and combo boxes for each control.
LRESULT CALLBACK ControlConfigWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // Create 10 combo boxes for 10 controls.
    static HWND hCombo[10];
    const char* controlLabels[10] = {
        "Power", "Reset", "A", "B", "Select", "Start", "Up", "Down", "Left", "Right"
    };

    switch (message) {
        case WM_CREATE: {
            int y = 20;
            int spacing = 30;
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            for (int i = 0; i < 10; i++) {
                // Create label.
                HWND hLabel = CreateWindow("STATIC", controlLabels[i],
                    WS_CHILD | WS_VISIBLE,
                    10, y, 100, 20,
                    hWnd, NULL, GetModuleHandle(NULL), NULL);
                SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
                // Create combo box.
                hCombo[i] = CreateWindow("COMBOBOX", NULL,
                    CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
                    120, y, 150, 200,
                    hWnd, (HMENU)(200 + i), GetModuleHandle(NULL), NULL);
                SendMessage(hCombo[i], WM_SETFONT, (WPARAM)hFont, TRUE);
                // Populate combo box with keys.
                for (int j = 0; j < keyListCount; j++) {
                    SendMessage(hCombo[i], CB_ADDSTRING, 0, (LPARAM)keyList[j]);
                }
                // Set default selection based on the current global binding.
                const char* currentKeyStr = "";
                switch(i) {
                    case 0: currentKeyStr = ScancodeToString(key_power); break;
                    case 1: currentKeyStr = ScancodeToString(key_reset); break;
                    case 2: currentKeyStr = ScancodeToString(key_a); break;
                    case 3: currentKeyStr = ScancodeToString(key_b); break;
                    case 4: currentKeyStr = ScancodeToString(key_select); break;
                    case 5: currentKeyStr = ScancodeToString(key_start); break;
                    case 6: currentKeyStr = ScancodeToString(key_up); break;
                    case 7: currentKeyStr = ScancodeToString(key_down); break;
                    case 8: currentKeyStr = ScancodeToString(key_left); break;
                    case 9: currentKeyStr = ScancodeToString(key_right); break;
                }
                for (int j = 0; j < keyListCount; j++) {
                    if (strcmp(keyList[j], currentKeyStr) == 0) {
                        SendMessage(hCombo[i], CB_SETCURSEL, j, 0);
                        break;
                    }
                }
                y += spacing;
            }
            // Create OK and Cancel buttons.
            CreateWindow("BUTTON", "OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                70, y, 80, 25,
                hWnd, (HMENU)300, GetModuleHandle(NULL), NULL);
            CreateWindow("BUTTON", "Cancel",
                WS_CHILD | WS_VISIBLE,
                170, y, 80, 25,
                hWnd, (HMENU)301, GetModuleHandle(NULL), NULL);
        }
        break;

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == 300) { // OK button clicked.
                char keyBuffer[64];
                for (int i = 0; i < 10; i++) {
                    int sel = SendMessage(hCombo[i], CB_GETCURSEL, 0, 0);
                    if (sel != CB_ERR) {
                        SendMessage(hCombo[i], CB_GETLBTEXT, sel, (LPARAM)keyBuffer);
                        SDL_Scancode sc = GetScancodeFromString(keyBuffer);
                        switch(i) {
                            case 0: key_power = sc; break;
                            case 1: key_reset = sc; break;
                            case 2: key_a = sc; break;
                            case 3: key_b = sc; break;
                            case 4: key_select = sc; break;
                            case 5: key_start = sc; break;
                            case 6: key_up = sc; break;
                            case 7: key_down = sc; break;
                            case 8: key_left = sc; break;
                            case 9: key_right = sc; break;
                        }
                    }
                }
                DestroyWindow(hWnd);
            } else if (id == 301) { // Cancel button clicked.
                DestroyWindow(hWnd);
            }
        }
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Function to display the configuration window.
void ShowConfigWindow(HWND parent) {
    const char* configClassName = "ConfigWindowClass";
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = ControlConfigWndProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = configClassName;
    RegisterClass(&wc);

    HWND hConfigWnd = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        configClassName,
        "Configure Controls",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        350, 400,
        parent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    if (!hConfigWnd) {
        MessageBox(parent, "Failed to create config window", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    ShowWindow(hConfigWnd, SW_SHOW);

    MSG msg;
    // Use a PeekMessage loop that captures messages for all windows.
    while (IsWindow(hConfigWnd)) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (!IsDialogMessage(hConfigWnd, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        Sleep(1);  // Yield a little CPU time.
    }
}

// Callback for handling Windows messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_FILE_LOADROM:
                    load_rom();
                    break;
                case ID_FILE_EXIT:
                    cleanup();
                    PostQuitMessage(0);
                    exit(0);    // This is odd, but it avoids having to select quit twice to close holbroowNES while a program is running. Wierd for now...
                    break;
                case ID_CONTROL_RESET:
                    if (nes_running) {
                        reset_nes(cpu, bus, ppu);
                    }
                    break;
                case ID_CONTROL_POWER:
                    if (nes_running) {
                        nes_running = false;
                    } else {
                        nes_running = true;
                    }
                    break;
                case ID_CONFIG_CONTROLS:
                    ShowConfigWindow(hwnd);
                    break;
                case ID_HELP_HELP:
                    MessageBox(hwnd,
                        "Help:\n\n"
                        "Keyboard Commands:\n"
                        "P - Power Off\n"
                        "R - Reset\n\n"
                        "Z - A Button\n"
                        "X - B Button\n\n"
                        "TAB - Select\n"
                        "ENTER - Start\n\n"
                        "Arrow Keys:\n"
                        "UP - Move Up\n"
                        "DOWN - Move Down\n"
                        "LEFT - Move Left\n"
                        "RIGHT - Move Right\n",
                        "holbroowNES Help",
                        MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_HELP_ABOUT:
                    MessageBox(hwnd,
                        "About holbroowNES (Alpha 1.0.0):\n\n"
                        "holbroowNES - By Will Holbrook (holbroow)\n"
                        "Written for the SCC300 Third Year Project "
                        "at Lancaster University (2024-2025)\n\n"
                        "This software is a NES emulator designed to replicate the functionality of the original "
                        "Nintendo Entertainment System.\n\n"
                        "Feel free to reach out to me @\n"
                        "https://www.linkedin.com/in/willholbrook/\n"
                        "https://github.com/holbroow",
                        "About holbroowNES",
                        MB_OK | MB_ICONINFORMATION);
                    break;
                    
            }
            break;

        case WM_DESTROY:
            cleanup();
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

    wc.hIcon = (HICON)LoadImage(
        wc.hInstance,                 // Handle to application instance
        "resources/icon.ico",         // Path to the .ico file
        IMAGE_ICON,                   // Type of image
        0, 0,                         // Default size (use dimensions from .ico file)
        LR_LOADFROMFILE               // Load icon from file
    );

    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        0, "SDLWindowClass", "holbroowNES",
        WS_OVERLAPPEDWINDOW, WINDOW_X_POS, WINDOW_Y_POS,
        WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!hwnd) {
        MessageBox(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Create menu bar
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hControlMenu = CreateMenu();
    HMENU hConfigMenu = CreateMenu();
    HMENU hHelpMenu = CreateMenu();

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_LOADROM, "Load ROM");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, "Quit");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hControlMenu, "Control");
    AppendMenu(hControlMenu, MF_STRING, ID_CONTROL_RESET, "Reset");
    AppendMenu(hControlMenu, MF_STRING, ID_CONTROL_POWER, "Power");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hConfigMenu, "Config");
    AppendMenu(hConfigMenu, MF_STRING, ID_CONFIG_CONTROLS, "Controls");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");
    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_HELP, "Help");
    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, "About");

    SetMenu(hwnd, hMenu);
}


// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Parse command-line arguments (if provided)
    if (strlen(lpCmdLine) > 0) {
        file_path = strdup(lpCmdLine); // Append the argument to file_path
    }

    // Initialise 'holbroowNES' window (If error occurs, exit program)
    if (init_window() == -1) {
        MessageBox(NULL, "Failed to initialize the application window.", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Initialise Display
    init_sdl_display();

    // If we took a file path previously from an argument, we can start the nes instantly
    if (file_path) {
        nes_running = true;
    }

    // Forever, we run the NES either runninng or non-running, within the holbroowNES application, handling it accordingly
    while(true) {
        // Frame / Keyboard Variables
        frame_duration_ms       = 16;                           // Store our target of ~60 FPS (16ms per frame)
        frame_start_time_ms     = SDL_GetTicks();               // Store the start time for initial frame
        state                   = SDL_GetKeyboardState(NULL);   // Configure a value to store keyboard's 'state' (what is/isn't pressed)

        // Blank the screen (useful after a shutdown / 'power-off')
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        // This will happen if:
        //  a: there is no file_path (provided .nes ROM)
        //  b: the nes is powered off by the menubar option
        while (!nes_running) {
            // Handle Windows messages
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    cleanup();
                    exit(0);
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }


        // Initialise the 'NES' with a Cartridge, Bus, PPU, and CPU
        init_nes();
        

        // Run the NES!
        while (nes_running) {
            // Handle Windows messages
            if (nes_cycles_passed % 100 == 0) {
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    if (msg.message == WM_QUIT) {
                        cleanup();
                        nes_running = false;
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            // Handle any key presses (controller activity)
            bus->controller[0] = 0x00;                                  // Initiate Controller 0 (1st controller)

            if (state[key_power])       nes_running = false;            // POWER    (Key P) This only powers OFF within this loop
            if (state[key_reset])       reset_nes(cpu, bus, ppu);       // RESET    (Key R)

            if (state[key_a])           bus->controller[0] |= 0x80;     // A        (Key Z)
            if (state[key_b])           bus->controller[0] |= 0x40;     // B        (Key X)

            if (state[key_select])      bus->controller[0] |= 0x20;     // Select   (Key SELECT)
            if (state[key_start])       bus->controller[0] |= 0x10;     // Start    (Key ENTER/RETURN)

            if (state[key_up])          bus->controller[0] |= 0x08;     // Up       (Key UP ARR)
            if (state[key_down])        bus->controller[0] |= 0x04;     // Down     (Key DOWN ARR)
            if (state[key_left])        bus->controller[0] |= 0x02;     // Left     (Key LEFT ARR)
            if (state[key_right])       bus->controller[0] |= 0x01;     // Right    (Key RIGHT ARR)


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
                update_sdl_display();
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

    }


    // Clean up - Free NES components + SDL/Window from memory
    // Nothing here and below should ever really happen... 
    // We are only ever in two states unless explicitly killed as a progrma process in Windows.
    cleanup();

    // Bye bye!
    return 0;
}
