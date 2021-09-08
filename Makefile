BIN=sdl-text-test

all:
	$(CC) main.c utf8.c -Wall -Wextra -lSDL2 -lSDL2_ttf -o $(BIN)

clean:
	$(RM) $(BIN)
