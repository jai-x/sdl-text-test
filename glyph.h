#include <stddef.h>
#include <SDL2/SDL.h>

typedef struct {
	char utf8[5];
	int w;
	int h;
	SDL_Texture* texture;
} glyph;

#define EMPTY_GLYPH (glyph){                  \
	.utf8 = { '\0', '\0', '\0', '\0', '\0',}, \
	.w = 0,                                   \
	.h = 0,                                   \
	.texture = NULL,                          \
}                                             \

/*
 * Returns the number of glyphs in the glyph array.
 */
size_t glyph_len(glyph* arr);

/*
 * Frees the glyph array, including the glyph textures.
 */
void glyph_free(glyph* arr);

/*
 * Returns a malloc'd string of UTF-8 encoded text that the glyph array
 * represents.
 */
char* glyph_to_string(glyph* arr);

/*
 * Append a UTF-8 encoded string to the glyph array.
 * Returns an updated pointer to the glyph array.
 */
glyph* glyph_append(glyph* arr, const char* utf8_str);

/*
 * Inserts a UTF-8 encoded string to the glyph array at the given index.
 * Returns an updated pointer to the glyph array.
 */
glyph* glyph_insert(glyph* arr, const size_t index, const char* utf8_str);

/*
 * Removes a the given count of glyphs from the glyph array starting from the
 * given index.
 * Returns an updated pointer to the glyph array.
 */
glyph* glyph_remove(glyph* arr, const size_t index, const size_t count);
