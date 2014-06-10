#ifndef GLIB_UTIL_H
#define GLIB_UTIL_H

#include <glib.h>


/*
 * Same as g_hash_table_destroy, but offers the option to automatically 
 * free the keys and values.
 */
void g_hash_table_free(GHashTable *hash_table, gboolean free_keys, gboolean free_values);


#endif
