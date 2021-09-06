#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdbool.h>

#include "utf8.h"

#define WIN_WIDTH 640
#define WIN_HEIGHT 200
#define USAGE "Usage: %s <font.ttf>\n"
#define SIZE 40
#define TEXTBOX_WIDTH (WIN_WIDTH - 50)
#define TEXTBOX_HEIGHT (SIZE + 2)
#define TEXTBOX_MARGIN_X 10
#define TEXTBOX_MARGIN_Y 5

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

SDL_Color white = { 255, 255, 255, 0 };
SDL_Color black = {   0,   0,   0, 0 };
SDL_Color red =   { 255,   0,   0, 0 };

int
cleanup(int status)
{
	if (TTF_WasInit()) {
		TTF_Quit();
	}

	if (SDL_WasInit(SDL_INIT_VIDEO)) {
		SDL_Quit();
	}

	return status;
}

void
SDL_SetRenderDrawColorType(SDL_Renderer* renderer, SDL_Color* color)
{
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
}

int
text_draw_width(TTF_Font* font, const char* text)
{
	int w = 0;
	int h = 0;
	TTF_SizeUTF8(font, text, &w, &h);
	return w;
}

int
main (int argc, char* argv[])
{
	if (argc < 2) {
		SDL_Log(USAGE, argv[0]);
		return EXIT_FAILURE;
	}

	// Init SDL TTF
	TTF_Init();

	// Load font
	TTF_Font* font = TTF_OpenFont(argv[1], SIZE);
	if (font == NULL) {
		SDL_Log("Error loading font %s (%dpt): %s\n", argv[1], SIZE, TTF_GetError());
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

	SDL_Rect textbox = {
		.x = (WIN_WIDTH - TEXTBOX_WIDTH) / 2,
		.y = (WIN_HEIGHT - TEXTBOX_HEIGHT) / 2,
		.w = TEXTBOX_WIDTH,
		.h = TEXTBOX_HEIGHT,
	};

	SDL_Rect textbox_focus = {
		.x = (WIN_WIDTH - TEXTBOX_WIDTH - 1) / 2,
		.y = (WIN_HEIGHT - TEXTBOX_HEIGHT - 2) / 2,
		.w = TEXTBOX_WIDTH + 2,
		.h = TEXTBOX_HEIGHT + 2,
	};

	SDL_Texture* text_texture = NULL;
	SDL_Rect text_rect;

	bool text_valid = false;
	char *text = NULL;
	int text_width = 0;

	bool focus_valid = false;
	bool focus = false;

	bool cursor_valid = false;
	size_t cursor_rune_index = 0;
	SDL_Rect cursor_rect = {
		.x = textbox.x + TEXTBOX_MARGIN_X,
		.y = textbox.y + TEXTBOX_MARGIN_Y,
		.w = 1,
		.h = textbox.h - (TEXTBOX_MARGIN_Y * 2),
	};

	bool alive = true;
	SDL_Event event;

	while (SDL_WaitEvent(&event)) {
		if (!alive) {
			break;
		}

		// --- Begin Inputs ---
		switch (event.type) {
			case SDL_QUIT:
				alive = false;
				break;

			case SDL_MOUSEBUTTONDOWN:
				bool new_focus = SDL_PointInRect(&(SDL_Point){ event.button.x, event.button.y }, &textbox);
				if (new_focus != focus) {
					focus = new_focus;
					focus_valid = false;
				}
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					if (focus) {
						focus = false;
						focus_valid = false;
					}
				}

				if (event.key.keysym.sym == SDLK_BACKSPACE) {
					if (focus && cursor_rune_index > 0) {
						text = utf8_remove(text, cursor_rune_index - 1, 1);
						cursor_rune_index--;
						text_valid = false;
						cursor_valid = false;
					}
				}

				if (event.key.keysym.sym == SDLK_LEFT) {
					if (focus && cursor_rune_index > 0) {
						cursor_rune_index--;
						cursor_valid = false;
					}
				}

				if (event.key.keysym.sym == SDLK_RIGHT) {
					if (focus && cursor_rune_index < utf8_rune_count(text)) {
						cursor_rune_index++;
						cursor_valid = false;
					}
				}

				if (event.key.keysym.sym == SDLK_v && (SDL_GetModState() & KMOD_CTRL)) {
					if (focus) {
						char* paste = SDL_GetClipboardText();
						if (paste != NULL) {
							text = utf8_insert(text, cursor_rune_index, paste);
							cursor_rune_index += utf8_rune_count(paste);
							SDL_free(paste);
							text_valid = false;
							cursor_valid = false;
						}
					}
				}
				break;

			case SDL_TEXTINPUT:
				if (focus) {
					text = utf8_insert(text, cursor_rune_index, event.text.text);
					cursor_rune_index += utf8_rune_count(event.text.text);
					text_valid = false;
					cursor_valid = false;
				}
				SDL_Log("Text Input Event: %s\n", event.text.text);
				break;

			case SDL_TEXTEDITING:
				SDL_Log("Text Editing Event: %s\n", event.edit.text);
				break;

			default:
				break;
		}
		// --- End Inputs ---

		// --- Begin Processing ---
		if (!focus_valid) {
			if (focus) {
				SDL_StartTextInput();
				SDL_Log("Start Text Input");
			} else {
				SDL_StopTextInput();
				SDL_Log("Stop Text Input");
			}

			focus_valid = true;
		}

		if (!text_valid) {
			if (text_texture != NULL) {
				SDL_DestroyTexture(text_texture);
			}

			if (text != NULL) {
				// Draw and position the text
				SDL_Surface* text_surface = TTF_RenderUTF8_Solid(font, text, black);
				text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
				text_rect = (SDL_Rect){
					.x = textbox.x + TEXTBOX_MARGIN_X,
					.y = textbox.y + TEXTBOX_MARGIN_Y,
					.w = text_surface->w,
					.h = text_surface->h,
				};
				text_width = text_surface->w;
				SDL_FreeSurface(text_surface);
			} else {
				text_width = 0;
			}

			text_valid = true;
			SDL_Log("Current Text: %s\n", text);
		}

		if (!cursor_valid) {
			int cursor_offset = 0;
			if (cursor_rune_index != 0) {
				char* text_before_cursor = utf8_runes_from_left(text, cursor_rune_index);
				cursor_offset = text_draw_width(font, text_before_cursor);
				free(text_before_cursor);
			}
			cursor_rect.x = textbox.x + TEXTBOX_MARGIN_X + cursor_offset;
			cursor_valid = true;
			SDL_Log("Cursor Rune Index: %zu\n", cursor_rune_index);
		}
		// --- End Processing ---

		// --- Begin Draw ---
		SDL_SetRenderDrawColorType(renderer, &white);
		SDL_RenderClear(renderer);

		if (text_texture != NULL) {
			SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
		}

		SDL_SetRenderDrawColorType(renderer, &black);
		SDL_RenderDrawRect(renderer, &textbox);

		if (focus) {
			SDL_SetRenderDrawColorType(renderer, &red);
			SDL_RenderDrawRect(renderer, &textbox_focus);
			SDL_SetRenderDrawColorType(renderer, &black);
			SDL_RenderFillRect(renderer, &cursor_rect);
		}

		SDL_RenderPresent(renderer);
		// --- End Draw ---
	}

	return cleanup(EXIT_SUCCESS);
}
