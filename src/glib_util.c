#include <stdlib.h>

#include "glib_util.h"


static void table_free_entry(gpointer key, gpointer value, gpointer data)
{
	int flags = (int)data;
	if (flags & 1)
		free(key);
	if (flags & 2)
		free(value);
}


void g_hash_table_free(GHashTable *hash_table, gboolean free_keys, gboolean free_values)
{
	g_hash_table_foreach(hash_table, table_free_entry, (gpointer)(free_keys | free_values<<1));
	g_hash_table_destroy(hash_table);
}

