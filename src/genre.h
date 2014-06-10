#ifndef GENRE_H
#define GENRE_H

#include "elist.h"


/*
 * Create a list with the genre names, optionally sorted.
 */
GEList *genre_create_list(gboolean sort);

/*
 * Gets the id of the genre with the given name. Case insensive.
 * Returns -1 if the name is not recognized.
 */
gint genre_get_id(const gchar *name);

/*
 * Gets the genre name given the id.
 * Returns NULL if the id is invalid.
 */
const gchar *genre_get_name(gint id);


#endif
