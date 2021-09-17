#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "glyph.h"

#define USAGE "Usage: %s <font.ttf>\n"

#define TEXT_SIZE 40

#define TEXTBOX_WIDTH 590
#define TEXTBOX_HEIGHT (TEXT_SIZE + 2)

#define TEXTBOX_PADDING_X 5
#define TEXTBOX_PADDING_Y 2

#define CURSOR_HEIGHT (TEXT_SIZE - 7)

static const SDL_Color white = { 255, 255, 255, 0 };
static const SDL_Color black = {   0,   0,   0, 0 };
static const SDL_Color red   = { 255,   0,   0, 0 };
static const SDL_Color gray  = { 128, 128, 128, 0 };

static TTF_Font* font = NULL;
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

static int window_width = 640;
static int window_height = 200;
static SDL_Rect textbox = { 0, 0, TEXTBOX_WIDTH, TEXTBOX_HEIGHT };

static bool focus = false;

static bool text_updated = true;
static glyph* text = NULL;
static glyph* composition = NULL;
static SDL_Texture* text_texture = NULL;
static SDL_Rect text_rect = {};

static bool cursor_updated = true;
static size_t cursor_glyph_index = 0;
static SDL_Rect cursor_rect = { 0, 0, 1, CURSOR_HEIGHT };

static void
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
handle_keydown(SDL_Keycode code)
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
			if (focus && cursor_glyph_index > 0) {
				text = glyph_remove(text, cursor_glyph_index - 1, 1);
				cursor_glyph_index--;
				text_updated = true;
				cursor_updated = true;
			}
			return;
		}

		case SDLK_DELETE: {
			if (focus && cursor_glyph_index < glyph_len(text)) {
				text = glyph_remove(text, cursor_glyph_index, 1);
				text_updated = true;
				cursor_updated = true;
			}
			return;
		}

		case SDLK_LEFT: {
			if (focus && cursor_glyph_index > 0) {
				cursor_glyph_index--;
				cursor_updated = true;
			}
			return;
		}

		case SDLK_RIGHT: {
			if (focus && cursor_glyph_index < glyph_len(text)) {
				cursor_glyph_index++;
				cursor_updated = true;
			}
			return;
		}
	}
}

size_t
get_closest_glyph_index(int x)
{
	size_t len = glyph_len(text);

	if (len == 0) {
		return 0;
	}

	if (x >= (text_rect.x + text_rect.w)) {
		return len;
	}

	if (x <= text_rect.x) {
		return 0;
	}

	int offset = 0;
	for (size_t i = 0; i < len; i++) {
		offset += (text[i].w / 2);

		if (x < (text_rect.x + offset)) {
			return i;
		}

		offset += (text[i].w / 2);
	}

	return len;
}

void
handle_mousedown(SDL_MouseButtonEvent evt)
{
	bool new_focus = SDL_PointInRect(&(SDL_Point){ evt.x, evt.y }, &textbox);
	if (new_focus != focus) {
		focus = new_focus;
		if (focus) {
			start_text_input();
			cursor_updated = true;
		} else {
			stop_text_input();
		}
	}

	if (focus) {
		cursor_glyph_index = get_closest_glyph_index(evt.x);
		cursor_updated = true;
	}
}

void
draw_textbox(void)
{
	// Recenter
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

static int
draw_glyph(glyph* g, int x)
{
	SDL_Rect glyph_rect = { .x = x, .y = 0, .w = g->w, .h = g->h };
	SDL_RenderCopy(renderer, g->texture, NULL, &glyph_rect);
	return x + g->w;
}

void
draw_text(void)
{
	// Resize based on textbox
	text_rect = (SDL_Rect){
		.x = textbox.x + 1 + TEXTBOX_PADDING_X,
		.y = textbox.y + 1 + TEXTBOX_PADDING_Y,
		.w = textbox.w - 2 - TEXTBOX_PADDING_X,
		.h = textbox.h - 2 - TEXTBOX_PADDING_Y,
	};

	// TODO: Figure out if this works?
	SDL_SetTextInputRect(&text_rect);

	if (text_updated) {
		size_t text_len = glyph_len(text);
		size_t composition_len = glyph_len(composition);

		// Ensure each glyph in composition has a texture
		if (composition_len > 0) {
			for (size_t i = 0; i < composition_len; i++) { 
				if (!composition[i].texture) {
					SDL_Surface* glyph_surface = TTF_RenderUTF8_Solid(font, composition[i].utf8, gray);

					composition[i].texture = SDL_CreateTextureFromSurface(renderer, glyph_surface);
					composition[i].w = glyph_surface->w;
					composition[i].h = glyph_surface->h;

					SDL_FreeSurface(glyph_surface);
				}
			};
		}

		// Ensure each glyph in text has a texture
		if (text_len > 0) {
			for (size_t i = 0; i < text_len; i++) { 
				if (!text[i].texture) {
					SDL_Surface* glyph_surface = TTF_RenderUTF8_Solid(font, text[i].utf8, black);

					text[i].texture = SDL_CreateTextureFromSurface(renderer, glyph_surface);
					text[i].w = glyph_surface->w;
					text[i].h = glyph_surface->h;

					SDL_FreeSurface(glyph_surface);
				}
			};
		}

		// Initialise texture it doesn't exist
		if (!text_texture) {
			text_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_UNKNOWN, SDL_TEXTUREACCESS_TARGET, text_rect.w, text_rect.h);
		}

		// Set render target to texture
		SDL_SetRenderTarget(renderer, text_texture);

		// Clear White
		SDL_SetRenderDrawColorType(renderer, &white);
		SDL_RenderClear(renderer);

		int x_offset = 0;

		// Draw text
		if (text_len > 0) {
			for (size_t i = 0; i < text_len; i++) {
				// Draw composition if it is inside or at the beginning of text
				if (i == cursor_glyph_index && composition_len > 0) {
					for (size_t c = 0; c < composition_len; c++) {
						x_offset = draw_glyph(&composition[c], x_offset);
					}
				}
				x_offset = draw_glyph(&text[i], x_offset);
			}
		}

		// Draw composition if it is at the end
		if (cursor_glyph_index == text_len && composition_len > 0) {
			for (size_t c = 0; c < composition_len; c++) {
				x_offset = draw_glyph(&composition[c], x_offset);
			}
		}

		// Set render target back to window
		SDL_SetRenderTarget(renderer, NULL);
		text_updated = false;

		char* glyph_str = glyph_to_string(text);
		SDL_Log("Current Text: %s\n", glyph_str);
		if (glyph_str) {
			free(glyph_str);
		}
	}

	if (text_texture) {
		SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
	}
}

void
draw_cursor(void)
{
	if (cursor_updated) {
		// Recenter
		cursor_rect.x = text_rect.x;
		cursor_rect.y = text_rect.y;

		int cursor_offset = 0;

		if (cursor_glyph_index != 0) {
			for (size_t i = 0; i < cursor_glyph_index; i++) {
				cursor_offset += text[i].w;
			}
		}

		cursor_rect.x += cursor_offset;

		cursor_updated = false;
		SDL_Log("Cursor Glyph Index: %zu\n", cursor_glyph_index);
	}

	if (focus) {
		SDL_SetRenderDrawColorType(renderer, &black);
		SDL_RenderFillRect(renderer, &cursor_rect);
	}
}

int
main(int argc, char* argv[])
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

	// text input is autostarted on desktop but we don't want that
	SDL_StopTextInput();

	// Print versions
	SDL_version version;
	SDL_GetVersion(&version);
	SDL_Log("Using SDL v%d.%d.%d\n", version.major, version.minor, version.patch);
	SDL_TTF_VERSION(&version);
	SDL_Log("Using SDL_TTF v%d.%d.%d\n", version.major, version.minor, version.patch);

	bool alive = true;
	SDL_Event e = {};

	// Main loop
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
				handle_mousedown(e.button);
				break;
			}

			case SDL_KEYDOWN: {
				handle_keydown(e.key.keysym.sym);
				break;
			}

			case SDL_TEXTINPUT: {
				if (focus) {
					size_t old_len = glyph_len(text);
					text = glyph_insert(text, cursor_glyph_index, e.text.text);
					size_t new_len = glyph_len(text);
					cursor_glyph_index += (new_len - old_len);

					text_updated = true;
					cursor_updated = true;
				}
				SDL_Log("Text Input Event: %s\n", e.text.text);
				break;
			}

			case SDL_TEXTEDITING: {
				if (focus) {
					if (composition) {
						glyph_free(composition);
						composition = NULL;
					}

					char* new_comp = e.edit.text;
					if (new_comp && new_comp[0]) {
						composition = glyph_insert(composition, 0, new_comp);
					}

					text_updated = true;
				}
				SDL_Log("Text Editing Event: text: %s, start: %d, length: %d timestamp: %d\n", e.edit.text, e.edit.start, e.edit.length, e.edit.timestamp);
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
		glyph_free(text);
	}

	if (text_texture) {
		SDL_DestroyTexture(text_texture);
	}

	TTF_CloseFont(font);
	SDL_Quit();
	TTF_Quit();

	return EXIT_SUCCESS;
}
