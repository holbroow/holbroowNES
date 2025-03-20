# holbroowNES

![C Language](https://img.shields.io/badge/language-C-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**Will Holbrook**  
*Lancaster University Third Year Project 2024*  
**Course:** Computer Science BSc (G400)  
**Module:** SCC 300 - EmuPC  
**Supervisor:** Professor Andrew Scott

## Overview

`holbroowNES` is an implementation of a fully functional NES emulator written in C. This project is part of the SCC 300 Third Year Project at Lancaster University, overseen by Professor Andrew Scott. Commit count is low due to a lot of changes on the fly during the development on my machine, at the point of 10 'commits' the project had likely undergone up to 200-300 small, seemingly insignificant changes privately on my machine.

holbroowNES emulates the classic NES by implementing:

- **CPU Emulation:** A full implementation of the NES’s 2A03 CPU (a variant of the MOS 6502) including all standard opcodes, and addressing modes.
- **PPU Emulation:** The 2C02 Picture Processing Unit (PPU) is emulated for background and sprite rendering, scrolling, and palette management.
- **Memory & Bus Architecture:** A dedicated bus connects the CPU, PPU, and Cartridge, with support for DMA transfers.
- **Cartridge & Mapper Support:** Load and run NES ROMs with Mapper 0 (NROM) support. The framework is also in place to add additional mappers (e.g., Mapper 1, 2, 3).
- **SDL2 Rendering:** Utilizes SDL2 to create a window and render the NES framebuffer at a scaled resolution.
- **Windows Integration:** Incorporates Windows-specific features (e.g., file open dialogs and menu-based controls) for an integrated user experience.

## Features

- **Accurate NES Hardware Emulation:** CPU, PPU, memory bus, and DMA.
- **Cartridge Loading:** Support for standard NES ROM formats (.nes, .rom, .bin) via an interactive file dialog.
- **User Interface:** Windows-based menu options for loading ROMs, resetting, and powering off the emulator.
- **Keyboard Controls:**
  - **P:** Power Off
  - **R:** Reset
  - **Z:** A Button
  - **X:** B Button
  - **TAB:** Select
  - **ENTER:** Start
  - **Arrow Keys:** Directional inputs


## Requirements

- **C Compiler:** A C compiler supporting C99 (or later).
- **SDL2 Library:** Make sure the SDL2 development libraries are installed.
- **Windows Environment:** The current implementation uses Windows-specific APIs (e.g., `<windows.h>`, `<commdlg.h>`). For other platforms, modifications may be necessary.

### Installation

1. **Clone the Repository:**
    ```bash
    git clone https://github.com/holbroow/holbroowNES.git
    ```
2. **Navigate to the Project Directory:**
    ```bash
    cd holbroowNES
    ```
3. **Compile the Project:**
    ```bash
    make
    ```

### Usage

Run the emulator using the compiled executable:
```bash
./holbroowNES.exe
