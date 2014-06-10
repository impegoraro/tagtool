#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "str_util.h"
#include "math_util.h"


/*** private functions ******************************************************/

static char *convert_sentence_case(const char *str)
{
	int result_size = strlen(str) + 12;
	char *result = malloc(result_size);
	const char *r = str;
	char *w = result;

	while (*r) {
		int len;
		gunichar c = g_utf8_get_char(r);

		/* ensure there is room for at least 1 more character */
		if (w - result + 7 > result_size) {
			int offset = w - result;
			result_size += max(6, 0.5 * result_size);
			result = realloc(result, result_size);
			w = result + offset;
		}

		if (r == str)
			len = g_unichar_to_utf8(g_unichar_toupper(c), w);
		else
			len = g_unichar_to_utf8(g_unichar_tolower(c), w);
		w += len;

		r = g_utf8_next_char(r);
	}

	*w = 0;
	
	return result;
}

static char *convert_title_case(const char *str)
{
	int result_size = strlen(str) + 12;
	char *result = malloc(result_size);
	const char *r = str;
	char *w = result;
	gboolean upcase_next = TRUE;

	while (*r) {
		int len;
		gunichar c = g_utf8_get_char(r);

		/* ensure there is room for at least 1 more character */
		if (w - result + 7 > result_size) {
			int offset = w - result;
			result_size += max(6, 0.5 * result_size);
			result = realloc(result, result_size);
			w = result + offset;
		}

		if (upcase_next)
			len = g_unichar_to_utf8(g_unichar_toupper(c), w);
		else
			len = g_unichar_to_utf8(g_unichar_tolower(c), w);
		w += len;

		upcase_next = !g_unichar_isalnum(c) && c != '\'';

		r = g_utf8_next_char(r);
	}

	*w = 0;
	
	return result;
}


/*** public functions *******************************************************/

char *str_convert_case(const char *str, int conv)
{
	switch (conv) {
		case CASE_CONV_NONE: {
			return strdup(str);
		}
		case CASE_CONV_LOWER: {
			return g_utf8_strdown(str, -1);
		}
		case CASE_CONV_UPPER: {
			return g_utf8_strup(str, -1);
		}
		case CASE_CONV_SENTENCE: {
			return convert_sentence_case(str);
		}
		case CASE_CONV_TITLE: {
			return convert_title_case(str);
		}
		default: {
			fprintf(stderr, "str_convert_case: unrecognized convertion type (%i)\n", conv);
			return NULL;
		}
	}
}


char *str_replace_char(const char *str, gunichar orig, gunichar repl)
{
	int result_size = strlen(str) + 6;
	char *result = malloc(result_size);
	const char *r = str;
	char *w = result;

	while (*r) {
		int len;
		gunichar c = g_utf8_get_char(r);

		/* ensure there is room for at least 1 more character */
		if (w - result + 7 > result_size) {
			int offset = w - result;
			result_size += max(6, 0.5 * result_size);
			result = realloc(result, result_size);
			w = result + offset;
		}

		if (c == orig)
			len = g_unichar_to_utf8(repl, w);
		else
			len = g_unichar_to_utf8(c, w);
		w += len;

		r = g_utf8_next_char(r);
	}

	*w = 0;

	return result;
}


char *str_remove_char(const char *str, gunichar rem)
{
	char *result = malloc(strlen(str) + 1);
	const char *r = str;
	char *w = result;

	while (*r) {
		gunichar c = g_utf8_get_char(r);
		if (c != rem) {
			int len = g_unichar_to_utf8(c, w);
			w += len;
		}

		r = g_utf8_next_char(r);
	}

	*w = 0;

	return result;
}

void str_rtrim(char *str)
{
	int len = strlen(str);
	char *last = str + len;
	char *curr = last - 1;

	if (len == 0)
		return;
	
	while (*curr == ' ' || *curr == '\t' || *curr == '\r' || *curr == '\n') {
		last = curr;
		curr = g_utf8_find_prev_char(str, curr);
		if (curr == NULL)
			break;
	}

	*last = 0;
}

void str_ltrim(char *str)
{
	char *pos = str;

	while (*pos && (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n')) {
		pos = g_utf8_find_next_char(pos, NULL);
	}

	if (pos != str) {
		size_t size = strlen(str) - (pos-str) + 1;
		memmove(str, pos, size);
	}
}

void str_trim(char *str)
{
	str_rtrim(str);
	str_ltrim(str);
}


char *str_strnchr(const char *s, char c, int n)
{
	int p = (int)s;
	int count = 0;

	while (*(char *)p) {
		if (*(char *)p == c)
			if (++count == n)
				return (char *)p;
		p++;
	}
	return NULL;
}

char *str_strnrchr(const char *s, char c, int n)
{
	int p = (int)s + strlen(s);
	int count = 0;

	while (p >= (int)s) {
		if (*(char *)p == c)
			if (++count == n)
				return (char *)p;
		p--;
	}
	return NULL;
}

char *str_safe_strncpy(char *dest, const char *src, size_t n)
{
	char *end;

	if (n <= 0)
		return NULL;

	strncpy(dest, src, n);
	dest[n-1] = 0;

	/* may have left an incomplete multibyte character at the end */
	if (!g_utf8_validate(dest, -1, (const char **)&end))
		*end = 0;

	return dest;
}


void str_ascii_tolower(char *str)
{
	char *p;
	for (p = str; *p ; p++)
		*p = g_ascii_tolower(*p);
}

void str_ascii_toupper(char *str)
{
	char *p;
	for (p = str; *p ; p++)
		*p = g_ascii_toupper(*p);
}


void str_ascii_replace_char(char *str, char orig, char repl)
{
	char *p;
	for (p = str; *p; p++)
		if (*p == orig)
			*p = repl;
}

