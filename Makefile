all:
	gcc -I sdl/include -L sdl/lib -o test src/Main.c src/holbroow6502.c src/Bus.c src/PPU.c src/Cartridge.c src/Mapper.c src/Mapper_0.c -lSDL2main -lSDL2