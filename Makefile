all:
	gcc -I sdl/include -L sdl/lib -o display Display.c -lSDL2main -lSDL2