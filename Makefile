all:
	gcc -I sdl/include -L sdl/lib resources/icon.res -o holbroowNES src/*.c -lSDL2main -lSDL2 -lcomdlg32 -lgdi32 -mwindows