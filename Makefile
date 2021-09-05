all:
	$(CC) main.c utf8.c -Wall -Wextra -lSDL2 -lSDL2_ttf -o sdl-text-test
