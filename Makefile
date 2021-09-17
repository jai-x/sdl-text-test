BIN=sdl-text-test
SRCS=main.c glyph.c
CFLAGS=-Wall -Wextra -std=c99
LIBS=-lSDL2 -lSDL2_ttf

all:
	$(CC) $(SRCS) $(CFLAGS) $(LIBS) -o $(BIN)

clean:
	$(RM) $(BIN)
