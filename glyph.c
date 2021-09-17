#include <stddef.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "glyph.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

static bool
is_utf8_codepoint_start(const char ch)
{
	//pkg.go.dev/unicode/utf8#RuneStart
	return (ch & 0xC0) != 0x80;
}

static size_t
utf8_codepoint_count(const char* str)
{
	if (str == NULL) {
		return 0;
	}

	int runes = 0;
	size_t len = strlen(str);
	for (size_t i = 0; i < len; i++) {
		if (is_utf8_codepoint_start(str[i])) {
			runes++;
		}
	}
	return runes;
}

size_t
glyph_len(glyph* arr)
{
	return arrlenu(arr);
}

void
glyph_free(glyph* arr)
{
	if (!arr) {
		return;
	}

	for (size_t i = 0; i < glyph_len(arr); i++) {
		if (arr[i].texture) {
			SDL_DestroyTexture(arr[i].texture);
		}
	}

	arrfree(arr);
}

char*
glyph_to_string(glyph* arr)
{
	if (glyph_len(arr) < 1) {
		return NULL;
	}

	char* out = calloc(glyph_len(arr) + 1, sizeof(char));
	size_t pos = 0;

	for (size_t g = 0; g < glyph_len(arr); g++) {
		size_t bytes = strlen(arr[g].utf8);
		for (size_t b = 0; b < bytes; b++) {
			out[pos++] = arr[g].utf8[b];
		}
	}

	return out;
}

glyph*
glyph_append(glyph* arr, const char* utf8_str)
{
	size_t len_bytes = 0;
	bool inside_rune = false;
	size_t start = 0;
	size_t end = 0;

	if (!utf8_str) {
		return arr;
	}

	len_bytes = strlen(utf8_str);
	if (len_bytes == 0) {
		return arr;
	}

	if (utf8_codepoint_count(utf8_str) == 0) {
		return arr;
	}

	for (size_t b = 0; b < len_bytes; b++) {
		if (is_utf8_codepoint_start(utf8_str[b])) {
			if (inside_rune) {
				end = b - 1;

				glyph g = EMPTY_GLYPH;
				for (size_t i = start; i <= end; i++) {
					g.utf8[i - start] = utf8_str[i];
				}
				arrput(arr, g);

				start = b;
			} else {
				start = b;
				inside_rune = true;
			}
		}
	}

	end = len_bytes - 1;

	glyph g = EMPTY_GLYPH;
	for (size_t i = start; i <= end; i++) {
		g.utf8[i - start] = utf8_str[i];
	}
	arrput(arr, g);

	return arr;
}

glyph*
glyph_insert(glyph* arr, const size_t index, const char* utf8_str)
{
	size_t len_bytes = 0;
	size_t current_rune = 0;
	bool inside_rune = false;
	size_t start = 0;
	size_t end = 0;

	if (index > arrlenu(arr)) {
		return glyph_append(arr, utf8_str);
	}

	if (!utf8_str) {
		return arr;
	}

	len_bytes = strlen(utf8_str);
	if (len_bytes == 0) {
		return arr;
	}

	if (utf8_codepoint_count(utf8_str) == 0) {
		return arr;
	}

	for (size_t b = 0; b < len_bytes; b++) {
		if (is_utf8_codepoint_start(utf8_str[b])) {
			if (inside_rune) {
				end = b - 1;

				glyph g = EMPTY_GLYPH;
				for (size_t i = start; i <= end; i++) {
					g.utf8[i - start] = utf8_str[i];
				}
				arrins(arr, index + current_rune, g);

				current_rune++;
				start = b;
			} else {
				start = b;
				inside_rune = true;
			}
		}

	}

	end = len_bytes - 1;

	glyph g = EMPTY_GLYPH;
	for (size_t i = start; i <= end; i++) {
		g.utf8[i - start] = utf8_str[i];
	}
	arrins(arr, index + current_rune, g);

	return arr;
}

glyph*
glyph_remove(glyph* arr, const size_t index, const size_t count)
{
	if (count < 1) {
		return arr;
	}

	if (index < 0 || index > arrlenu(arr) - 1) {
		return arr;
	}

	for (size_t i = index; i < (index + count); i++) {
		if (arr[i].texture) {
			SDL_DestroyTexture(arr[i].texture);
		}
	}

	arrdeln(arr, index, count);

	return arr;
}
