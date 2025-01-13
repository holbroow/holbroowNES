all:
	gcc -I sdl/include -L sdl/lib -o test src/*.c -lSDL2main -lSDL2