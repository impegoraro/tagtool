#ifndef STR_UTIL_H
#define STR_UTIL_H


/*
 * IMPORTANT NOTES:
 *
 * All functions manipulate UTF-8 strings, unless otherwise noted.
 *
 * All input and output strings are 0 terminated.
 *
 * All input strings are assumed to be valid. If the source of the string 
 * is suspect it should be validated first with g_utf8_validate().
 */


/* types of case conversion for str_convert_case() */
enum {
	CASE_CONV_NONE     = 0,
	CASE_CONV_LOWER    = 1,
	CASE_CONV_UPPER    = 2,
	CASE_CONV_SENTENCE = 3,
	CASE_CONV_TITLE    = 4
};


/*
 * Converts a string to the requested case.
 * The result is returned in a newly allocated string.
 */
char *str_convert_case(const char *str, int conv);

/*
 * Replaces all occurrences of the character <orig> with <repl>
 * The result is returned in a newly allocated string.
 */
char *str_replace_char(const char *str, gunichar orig, gunichar repl);

/*
 * Removes all occurrences of character <rem> from <str>.
 * The result is returned in a newly allocated string.
 */
char *str_remove_char(const char *str, gunichar rem);

/* 
 * Trims trailing whitespace characters from string <str>
 */
void str_rtrim(char *str);

/* 
 * Trims leading whitespace characters from string <str>
 */
void str_ltrim(char *str);

/* 
 * Trims leading and trailing whitespace characters from string <str>
 */
void str_trim(char *str);

/*
 * Returns a pointer to the <n>th occurrence of <c> in <s>, counting from 
 * the beginning.
 * Similar to strchr(3)
 */
char *str_strnchr(const char *s, char c, int n);

/*
 * Returns a pointer to the <n>th occurrence of <c> in <s>, counting from 
 * the end.
 * Similar to strrchr(3)
 */
char *str_strnrchr(const char *s, char c, int n);

/*
 * Safe version of strncpy, string is always NULL terminated.
 */
char *str_safe_strncpy(char *dest, const char *src, size_t n);


/*
 * Converts a pure ASCII string (characters 0x00 to 0x7f) to lower case. 
 * Conversion is done in-place.
 */
void str_ascii_tolower(char *str);

/*
 * Converts a pure ASCII string (characters 0x00 to 0x7f) to upper case. 
 * Conversion is done in-place.
 */
void str_ascii_toupper(char *str);

/*
 * Replaces all occurrences of the ASCII character <orig> with <repl>.
 * Replacement is done in-place.
 */
void str_ascii_replace_char(char *str, char orig, char repl);


#endif
