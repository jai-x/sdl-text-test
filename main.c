#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdbool.h>

#include "utf8.h"

#define USAGE "Usage: %s <font.ttf>\n"

#define TEXT_SIZE 40

#define TEXTBOX_WIDTH 590
#define TEXTBOX_HEIGHT (TEXT_SIZE + 2)

#define TEXTBOX_PADDING_X 10
#define TEXTBOX_PADDING_Y 5

#define CURSOR_HEIGHT (TEXT_SIZE - 7)

static const SDL_Color white = { 255, 255, 255, 0 };
static const SDL_Color black = {   0,   0,   0, 0 };
static const SDL_Color red =   { 255,   0,   0, 0 };

static TTF_Font* font = NULL;
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static int window_width = 640;
static int window_height = 200;
static SDL_Rect textbox = { 0, 0, TEXTBOX_WIDTH, TEXTBOX_HEIGHT };

static bool focus = false;

static bool text_updated = true;
static char *text = NULL;
static SDL_Texture* text_texture = NULL;
static SDL_Rect text_rect = {};

static bool cursor_updated = true;
static size_t cursor_rune_index = 0;
static SDL_Rect cursor_rect = { 0, 0, 1, CURSOR_HEIGHT };

static bool alive = true;

static SDL_Event e = {};

void
SDL_SetRenderDrawColorType(SDL_Renderer* renderer, const SDL_Color* color)
{
	SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
}

int
text_draw_width(const char* text)
{
	int w = 0;
	int h = 0;
	TTF_SizeUTF8(font, text, &w, &h);
	return w;
}

void
start_text_input(void)
{
	SDL_StartTextInput();
	SDL_Log("Start Text Input");
}

void
stop_text_input(void)
{
	SDL_StopTextInput();
	SDL_Log("Stop Text Input");
}

void
process_key(SDL_Keycode code)
{
	switch (code) {
		case SDLK_ESCAPE: {
			if (focus) {
				focus = false;
				stop_text_input();
			}
			return;
		}

		case SDLK_BACKSPACE: {
			if (focus && cursor_rune_index > 0) {
				text = utf8_remove(text, cursor_rune_index - 1, 1);
				cursor_rune_index--;
				text_updated = true;
				cursor_updated = true;
			}
			return;
		}

		case SDLK_DELETE: {
			if (focus && cursor_rune_index < utf8_rune_count(text)) {
				text = utf8_remove(text, cursor_rune_index, 1);
				text_updated = true;
				cursor_updated = true;
			}
			return;
		}

		case SDLK_LEFT: {
			if (focus && cursor_rune_index > 0) {
				cursor_rune_index--;
				cursor_updated = true;
			}
			return;
		}

		case SDLK_RIGHT: {
			if (focus && cursor_rune_index < utf8_rune_count(text)) {
				cursor_rune_index++;
				cursor_updated = true;
			}
			return;
		}
	}
}

void
draw_textbox(void)
{
	// recenter
	textbox.x = (window_width - textbox.w) / 2;
	textbox.y = (window_height - textbox.h) / 2;

	// draw
	SDL_SetRenderDrawColorType(renderer, &black);
	SDL_RenderDrawRect(renderer, &textbox);

	if (focus) {
		// draw focus border
		SDL_SetRenderDrawColorType(renderer, &red);
		SDL_RenderDrawRect(renderer, &(SDL_Rect){
			.x = textbox.x - 1,
			.y = textbox.y - 1,
			.w = textbox.w + 2,
			.h = textbox.h + 2,
		});
	}
}

void
draw_text(void)
{
	if (text_updated) {
		if (text_texture) {
			SDL_DestroyTexture(text_texture);
		}

		if (text) {
			// Render text to surface
			SDL_Surface* text_surface = TTF_RenderUTF8_Solid(font, text, black);

			// Output to texture
			text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);

			// Create rect of draw position on screen
			text_rect = (SDL_Rect){
				.x = textbox.x + TEXTBOX_PADDING_X,
				.y = textbox.y + TEXTBOX_PADDING_Y,
				.w = text_surface->w,
				.h = text_surface->h,
			};

			// free surface
			SDL_FreeSurface(text_surface);
		}

		text_updated = false;
		SDL_Log("Current Text: %s\n", text);
	}

	if (text_texture) {
		SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
	}
}

void
draw_cursor(void)
{
	if (cursor_updated) {
		cursor_rect.x = textbox.x + TEXTBOX_PADDING_X;
		cursor_rect.y = textbox.y + TEXTBOX_PADDING_Y;

		int cursor_offset = 0;

		if (cursor_rune_index != 0) {
			char* text_before_cursor = utf8_runes_from_left(text, cursor_rune_index);
			cursor_offset = text_draw_width(text_before_cursor);
			free(text_before_cursor);
		}

		cursor_rect.x += cursor_offset;

		cursor_updated = false;
		SDL_Log("Cursor Rune Index: %zu\n", cursor_rune_index);
	}

	if (focus) {
		SDL_SetRenderDrawColorType(renderer, &black);
		SDL_RenderFillRect(renderer, &cursor_rect);
	}
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
	font = TTF_OpenFont(argv[1], TEXT_SIZE);
	if (font == NULL) {
		SDL_Log("Error loading font %s (%dpt): %s\n", argv[1], TEXT_SIZE, TTF_GetError());
		TTF_Quit();
		return EXIT_FAILURE;
	}

	// Setup font
	TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
	TTF_SetFontOutline(font, 0);
	TTF_SetFontKerning(font, 1);
	TTF_SetFontHinting(font, TTF_HINTING_NORMAL);

	// Init SDL
	SDL_Init(SDL_INIT_VIDEO);
	int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	SDL_CreateWindowAndRenderer(window_width, window_height, flags, &window, &renderer);
	SDL_SetWindowTitle(window, "SDL Text Test");

	while (SDL_WaitEvent(&e)) {
		if (!alive) {
			break;
		}

		// --- Begin Inputs ---
		switch (e.type) {
			case SDL_QUIT: {
				alive = false;
				break;
			}

			case SDL_WINDOWEVENT: {
				if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
					window_width = e.window.data1;
					window_height = e.window.data2;

					text_updated = true;
					cursor_updated = true;
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN: {
				SDL_Point mouse_point = { e.button.x, e.button.y };
				bool new_focus = SDL_PointInRect(&mouse_point, &textbox);
				if (new_focus != focus) {
					focus = new_focus;
					focus ? start_text_input() : stop_text_input();
					cursor_updated = focus;
				}
				break;
			}

			case SDL_KEYDOWN: {
				process_key(e.key.keysym.sym);
				break;
			}

			case SDL_TEXTINPUT: {
				if (focus) {
					text = utf8_insert(text, cursor_rune_index, e.text.text);
					cursor_rune_index += utf8_rune_count(e.text.text);

					text_updated = true;
					cursor_updated = true;
				}
				SDL_Log("Text Input Event: %s\n", e.text.text);
				break;
			}

			case SDL_TEXTEDITING: {
				SDL_Log("Text Editing Event: %s\n", e.edit.text);
				break;
			}
		}
		// --- End Inputs ---

		// --- Begin Draw ---
		SDL_SetRenderDrawColorType(renderer, &white);
		SDL_RenderClear(renderer);

		draw_textbox();
		draw_text();
		draw_cursor();

		SDL_RenderPresent(renderer);
		// --- End Draw ---
	}

	if (text) {
		free(text);
	}

	if (text_texture) {
		SDL_DestroyTexture(text_texture);
	}

	TTF_CloseFont(font);
	SDL_Quit();
	TTF_Quit();

	return EXIT_SUCCESS;
}
