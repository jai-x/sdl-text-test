BIN=sdl-text-test
SRCS=main.c utf8.c
CFLAGS=-Wall -Wextra
LIBS=-lSDL2 -lSDL2_ttf

all:
	$(CC) $(SRCS) $(CFLAGS) $(LIBS) -o $(BIN)

clean:
	$(RM) $(BIN)
