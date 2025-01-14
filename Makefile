all:
	gcc -I sdl/include -L sdl/lib -o test src/*.c -lSDL2main -lSDL2 -lcomdlg32 -lgdi32