all:
	gcc -o test src/Main.c src/holbroow6502.c src/Bus.c src/PPU.c src/Cartridge.c -O3