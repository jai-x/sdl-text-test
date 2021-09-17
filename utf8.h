#include <stdbool.h>
#include <stdlib.h>

/*
 * Create a stack allocated UTF8 char* from a string literal.
 */
char* utf8_from_literal(const char* str);

/*
 * Returns if a byte is a UTF8 rune or is the start of one.
 */
bool utf8_is_rune_start(const char ch);

/*
 * Returns the count of UTF8 runes in the string
 */
size_t utf8_rune_count(const char* str);

/*
 * Appends the contents of src to dest.
 * Automatically resizes dest and returns it.
 */
char* utf8_append(char* dest, const char* src);

/*
 * Prepends the contents of src to dest.
 * Automatically resizes dest and returns it.
 */
char* utf8_prepend(char* dest, const char* src);


/*
 * Finds the byte offset corresponding to the given index of the UTF8 rune in
 * the string.
 */
size_t utf8_rune_to_byte_index(char* str, size_t rune_index);

/*
 * Returns a substring of runes from the string start.
 */
char* utf8_runes_from_left(char* str, size_t rune_index);

/*
 * Inserts str into dest starting from the given UTF8 rune index.
 * Automatically resizes dest and returns it.
 */
char* utf8_insert(char* dest, const size_t rune_index, const char* src);

/*
 * Removes a given count of UTF8 runes from dest starting from the given index.
 * Automatically resizes dest and returns it.
 */
char* utf8_remove(char* dest, const size_t rune_index, const size_t rune_count);
