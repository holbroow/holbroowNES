CC := C:/raylib/w64devkit/bin/gcc.exe

all:
	$(CC) -IC:/raylib/raylib/src -LC:/raylib/raylib/src -o test src/Main.c src/holbroow6502.c src/Bus.c src/PPU.c src/Cartridge.c -lraylib
