#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdbool.h>

#define WIN_WIDTH 640
#define WIN_HEIGHT 200
#define USAGE "Usage: %s <font.ttf> [size] [text]\n"
#define DEFAULT_SIZE 40
#define DEFAULT_TEXT "The quick brown fox jumps over the lazy dog 日本語"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Color white = { 255, 255, 255, 0 };
SDL_Color black = {   0,   0,   0, 0 };
TTF_Font* font = NULL;
char *text = NULL;
char* font_path = NULL;
int font_size;


int
cleanup(int status)
{
	if (font) {
		TTF_CloseFont(font);
	}

	if (TTF_WasInit()) {
		TTF_Quit();
	}

	if (SDL_WasInit(SDL_INIT_VIDEO)) {
		SDL_Quit();
	}

	return status;
}

int
main (int argc, char* argv[])
{
	if (argc < 2) {
		SDL_Log(USAGE, argv[0]);
		return EXIT_FAILURE;
	}

	// Assign command line arguments
	font_path = argv[1];
	font_size = DEFAULT_SIZE;
	text = DEFAULT_TEXT;

	// Init SDL TTF
	TTF_Init();

	// Load font
	font = TTF_OpenFont(font_path, font_size);
	if (font == NULL) {
		SDL_Log("Error loading font %s (%dpt): %s\n", font_path, font_size, TTF_GetError());
		return cleanup(EXIT_FAILURE);
	}

	// Setup font
	TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
	TTF_SetFontOutline(font, 0);
	TTF_SetFontKerning(font, 1);
	TTF_SetFontHinting(font, TTF_HINTING_NORMAL);

	// Init SDL
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);

	SDL_Surface* text_surface = TTF_RenderUTF8_Solid(font, text, black);
	SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
	SDL_Rect text_rect = {
		.x = (WIN_WIDTH - text_surface->w) / 2,
		.y = (WIN_HEIGHT - text_surface->h) / 2,
		.w = text_surface->w,
		.h = text_surface->h,
	};
	SDL_FreeSurface(text_surface);

	bool alive = true;
	SDL_Event event;
	while (SDL_WaitEvent(&event)) {
		if (!alive) {
			break;
		}

		switch (event.type) {
			case SDL_QUIT:
				alive = false;

			default:
				break;
		}

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
		SDL_RenderPresent(renderer);
	}

	return cleanup(EXIT_SUCCESS);
}
