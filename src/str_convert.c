#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <langinfo.h>
#include <errno.h>
#include <math.h>
#include <glib.h>

#include "str_convert.h"
#include "math_util.h"


/*** private functions ******************************************************/

static const char *encoding_name(int enc)
{
	static char *names[] = {
		"UTF-8",
		"UTF-16",
		"UTF-16BE",
		"UTF-16LE",
		"ISO-8859-1"
	};

	if (enc == LOCALE)
		return nl_langinfo(CODESET);
	else
		return names[enc];
}


/*
 * Returns the (minimum) number of bytes per character in this encoding.
 * This is necessary to find the end of zero-terminated strings (e.g. an 
 * UTF-8 string ends on a single zero byte, an UTF-16 string ends on 2 zero
 * bytes, etc.)
 *
 * XXX - Don't know a generic way to determine bytes per character for all 
 *       encodings.  For now multibyte encodings other than UTF will be wrong.
 */
static int bytes_per_char(int enc)
{
	if (strncasecmp(encoding_name(enc), "UTF-16", 6) == 0)
		return 2;
	else if (strncasecmp(encoding_name(enc), "UTF-32", 6) == 0)
		return 4;
	else
		return 1;
}


/*
 * Determines the size in bytes of a zero-terminated string, given the encoding.
 * Size includes any markers (Unicode BOM, terminating zero byte(s), etc.)
 *
 * XXX - Result will be wrong in those cases where bytes_per_char() is wrong.
 */
static size_t strsize(int enc, const void *str)
{
	const void *p = str;
	int bpc = bytes_per_char(enc);

	if (bpc != 1 && bpc != 2 && bpc != 4) {
		fprintf(stderr, "strsize: can't handle %i bytes per char\n", bpc);
		return 0;
	}

	while(1) {
		if ( (bpc == 1 && *(guint8 *)p == 0) ||
		     (bpc == 2 && *(guint16*)p == 0) ||
		     (bpc == 4 && *(guint32*)p == 0) )
			break;
		p += bpc;
	}

	return p - str + bpc;
}


/*** public functions *******************************************************/

char *str_convert_encoding(int from, int to, const char *str)
{
	/*
	 * And here it should just be a matter of calling glib's g_convert().
	 * Or so I thought.  Alas, they chickened out of the hard part: how to 
	 * figure out the size of a zero-terminaded string in any arbitrary 
	 * encoding.
	 * Since there is no advantage in using g_convert(), might as well 
	 * keep my old implementation.  It's worth using their iconv wrapppers 
	 * though, because they provide libiconv in systems that don't have it 
	 * natively.
	 */


	GIConv conv;
	char *result;
	char *inbuf, *outbuf;
	size_t inbpc, outbpc;
	size_t inbytes, outbytes;
	size_t inbytesleft, outbytesleft;
	size_t res;

	inbytes = strsize(from, str);

	if (strcasecmp(encoding_name(to), encoding_name(from)) == 0) {
		result = malloc(inbytes);
		memcpy(result, str, inbytes);
		return result;
	}

	conv = g_iconv_open(encoding_name(to), encoding_name(from));
	if (conv == (GIConv)-1) {
		fprintf(stderr, "convert_encoding: cannot convert from %s to %s\n", 
			encoding_name(from), encoding_name(to));
		return NULL;
	}

	inbpc = bytes_per_char(from);
	outbpc = bytes_per_char(to);

	/* estimate the converted size */
	outbytes = ((double)outbpc / (double)inbpc) * inbytes;
	/* optimize common cases (tuned for western european languages) */
	if (to == UTF_8 && inbpc == 1)
		outbytes = ceil(1.25 * inbytes);
	else if (to == UTF_16)
		outbytes += 2;	/* for the BOM */

	//printf("inbytes : %i\noutbytes: %i\n", inbytes, outbytes);

	result = malloc(outbytes);

	inbuf = (char*)str;
	inbytesleft = inbytes;
	outbuf = result;
	outbytesleft = outbytes;

	while(1) {
		res = g_iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
		if (res == (size_t)-1) {
			if (errno == E2BIG) {
				/* Ran out of space, alloc more 
				   This code tries hard to avoid the need for a second realloc,
				   while still keeping over-allocation to a minimum */
				double done = 1.0 - (double)inbytesleft / (double)inbytes;
				size_t bytes_written = outbuf - result;
				size_t newsize = ceil((bytes_written / done) * 1.1);

				//printf("growing: done=%g%%, old size=%i, new size=%i\n",
				//	100.0*done, outbytes, newsize);

				outbytesleft += newsize - outbytes;
				outbytes = newsize;
				result = realloc(result, outbytes);
				outbuf = result + bytes_written;

				continue;
			}
			else {
				/* Invalid or inconvertible char, skip it
				   Seems better than aborting the conversion... */

				fprintf(stderr, "convert_encoding: conversion error at offset %i\n", 
					inbytes-inbytesleft);

				inbuf += inbpc;
				inbytesleft = max(inbytesleft - inbpc, 0);
				outbuf += outbpc;
				outbytesleft = max(outbytesleft - outbpc, 0);

				continue;
			}
		}
		break;
	}

	//printf("%i of %i bytes unused (wasted %g%%)\n", 
	//	outbytesleft, outbytes, 100.0*(double)outbytesleft/(double)outbytes);

	g_iconv_close(conv);
	return result;
}


char *str_filename_to_utf8(const char *str, const char *onerror)
{
	char *result;

	result = g_filename_to_utf8(str, -1, NULL, NULL, NULL);
	if (result == NULL) {
		g_warning("File name could not be converted to UTF8: %s", str);
		if (onerror != NULL)
			result = strdup(onerror);
		else
			result = NULL;
	}
	return result;
}


char *str_filename_from_utf8(const char *str, const char *onerror)
{
	char *result;

	result = g_filename_from_utf8(str, -1, NULL, NULL, NULL);
	if (result == NULL) {
		g_warning("File name could not be converted from UTF8: %s", str);
		if (onerror != NULL)
			result = strdup(onerror);
		else
			result = NULL;
	}
	return result;
}

