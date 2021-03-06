#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Copied from musl libc
static char*
utf8_strdup(const char* str)
{
	size_t len = strlen(str);
	char* dest = malloc(len + 1);

	if (!dest) {
		return NULL;
	}

	return memcpy(dest, str, len + 1);
}

// Copied from musl libc
static size_t
utf8_strnlen(const char *s, size_t n)
{
	const char *p = memchr(s, 0, n);

	if (p) {
		return p - s;
	} else {
		return n;
	}
}

// Copied from musl libc
static char*
utf8_strndup(const char* str, size_t n)
{
	size_t len = utf8_strnlen(str, n);
	char* dest = malloc(len + 1);

	if (!dest) {
		return NULL;
	}

	memcpy(dest, str, len);
	dest[len] = '\0';
	return dest;
}

char*
utf8_from_literal(const char* str)
{
	return utf8_strdup(str);
}

bool
utf8_is_rune_start(const char ch)
{
	//pkg.go.dev/unicode/utf8#RuneStart
	return (ch & 0xC0) != 0x80;
}

size_t
utf8_rune_count(const char* str)
{
	if (str == NULL) {
		return 0;
	}

	int runes = 0;
	size_t len = strlen(str);
	for (size_t i = 0; i < len; i++) {
		if (utf8_is_rune_start(str[i])) {
			runes++;
		}
	}
	return runes;
}

char*
utf8_append(char* dest, const char* src)
{
	if (dest == NULL) {
		return utf8_from_literal(src);
	}

	size_t len = strlen(dest) + strlen(src);
	dest = (char*) realloc(dest, len + 1);
	return strcat(dest, src);
}

char*
utf8_prepend(char* dest, const char* src)
{
	char* after = utf8_strdup(dest);
	size_t after_size = strlen(after);

	const char* before = src;
	size_t before_size = strlen(before);

	dest = (char*) realloc(dest, before_size + after_size + 1);
	sprintf(dest, "%s%s", before, after);

	free(after);

	return dest;
}

size_t
utf8_rune_to_byte_index(const char* str, size_t rune_index)
{
	if (rune_index == 0 && utf8_is_rune_start(str[0])) {
		return 0;
	}

	size_t byte_index = 0;
	size_t byte_size = strlen(str);
	size_t current_rune_index = 0;

	for (; byte_index < byte_size; byte_index++) {
		// Skip first char since it's zero in both cases
		if (byte_index == 0) {
			continue;
		}

		if (utf8_is_rune_start(str[byte_index])) {
			current_rune_index++;
		}

		if (current_rune_index == rune_index) {
			return byte_index;
		}
	}

	return byte_size;
}

char*
utf8_runes_from_left(const char* str, size_t rune_index)
{
	size_t byte_index = utf8_rune_to_byte_index(str, rune_index);
	return utf8_strndup(str, byte_index);
}

char*
utf8_insert(char* dest, const size_t rune_index, const char* src)
{
	if (dest == NULL) {
		return utf8_from_literal(src);
	}

	if (rune_index < 1) {
		return utf8_prepend(dest, src);
	}

	if (rune_index > utf8_rune_count(dest) - 1) {
		return utf8_append(dest, src);
	}

	// find byte index at point to insert
	size_t byte_index = utf8_rune_to_byte_index(dest, rune_index);

	// split dest into before and after the byte index point
	char* before = utf8_strndup(dest, byte_index);
	size_t before_size = strlen(before);

	char* after = utf8_strdup(dest + byte_index);
	size_t after_size = strlen(after);

	// make the new string the middle
	const char* middle = src;
	size_t middle_size = strlen(middle);

	// resize dest to account for middle
	dest = (char*) realloc(dest, before_size + middle_size + after_size + 1);

	// shove before, middle, after into dest
	sprintf(dest, "%s%s%s", before, middle, after);

	// free temp before and after
	free(before);
	free(after);

	return dest;
}

char*
utf8_remove(char* dest, const size_t rune_index, const size_t rune_count)
{
	if (dest == NULL) {
		return NULL;
	}

	if (rune_count < 1) {
		return dest;
	}

	if (rune_index > utf8_rune_count(dest) - 1) {
		return dest;
	}

	// find byte index at point to remove from
	size_t byte_index = utf8_rune_to_byte_index(dest, rune_index);

	// find amount of bytes to remove from the byte index onwards
	size_t bytes_to_remove = 0;
	size_t byte_size = strlen(dest);
	size_t remaining_runes = rune_count + 1;
	for (size_t i = byte_index; i < byte_size; i++) {
		if (utf8_is_rune_start(dest[i])) {
			remaining_runes--;

			if (remaining_runes == 0) {
				break;
			}
		}

		bytes_to_remove++;
	}

	// take chars before the byte index
	char* before = utf8_strndup(dest, byte_index);
	size_t before_size = strlen(before);

	// take cahrs after the bytes index and the addition bytes to remove
	char* after = utf8_strdup(dest + byte_index + bytes_to_remove);
	size_t after_size = strlen(after);

	// resize and insert
	dest = (char*) realloc(dest, before_size + after_size + 1);
	sprintf(dest, "%s%s", before, after);

	// free temp variables
	free(before);
	free(after);

	// zero length string is useless
	if (dest[0] == '\0') {
		free(dest);
		return NULL;
	}

	return dest;
}
