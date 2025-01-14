#define SDL_MAIN_HANDLED

#include <windows.h>
#include <SDL2/SDL.h>

// Global variables for SDL
SDL_Window *sdlWindow = NULL;
SDL_Renderer *sdlRenderer = NULL;

// Menu IDs
#define ID_FILE_EXIT 9001
#define ID_HELP_ABOUT 9002

// Callback for handling Windows messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
                    break;
                case ID_HELP_ABOUT:
                    MessageBox(hwnd, "SDL2 Window with Menu Bar Example", "About", MB_OK | MB_ICONINFORMATION);
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

int main(int argc, char *argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        MessageBox(NULL, "Failed to initialize SDL", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Create the main Windows window
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "SDLWindowClass";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, "SDLWindowClass", "SDL2 with Menu Bar",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!hwnd) {
        MessageBox(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Create menu bar
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hHelpMenu = CreateMenu();

    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, "Exit");
    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, "About");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, "Help");

    SetMenu(hwnd, hMenu);

    // Create SDL2 window from the HWND
    sdlWindow = SDL_CreateWindowFrom((void*)hwnd);
    if (!sdlWindow) {
        MessageBox(NULL, "Failed to create SDL window", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!sdlRenderer) {
        MessageBox(NULL, "Failed to create SDL renderer", "Error", MB_OK | MB_ICONERROR);
        SDL_DestroyWindow(sdlWindow);
        return -1;
    }

    ShowWindow(hwnd, SW_SHOW);

    // Main loop
    MSG msg;
    int running = 1;

    while (running) {
        // Handle Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // SDL rendering
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 255, 255);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderPresent(sdlRenderer);

        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();

    return 0;
}
