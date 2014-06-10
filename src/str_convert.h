#ifndef STR_CONVERT_H
#define STR_CONVERT_H


/* text encodings */
enum {
	LOCALE     = -1,	// whatever the locale default is
	UTF_8      = 0,		// UTF-8
	UTF_16     = 1,		// UTF-16 with BOM
	UTF_16BE   = 2,		// UTF-16 big-endian, without BOM
	UTF_16LE   = 3,		// UTF-16 little-endian, without BOM
	ISO_8859_1 = 4		// ISO-8859-1, aka LATIN1
};


/*
 * Convert a string from one encoding to another
 * Returns a newly allocated string or NULL on failure
 */
char *str_convert_encoding(int from, int to, const char *str);


/*
 * Simplified wrapper to g_filename_to_utf8().
 * If there is a conversion error, <onerror> is returned instead of the 
 * converted file name.
 * The return value must be freed by the caller.
 */
char *str_filename_to_utf8(const char *str, const char *onerror);


/*
 * Simplified wrapper to g_filename_from_utf8().
 * If there is a conversion error, <onerror> is returned instead of the 
 * converted file name.
 * The return value must be freed by the caller.
 */
char *str_filename_from_utf8(const char *str, const char *onerror);


#endif
