#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "elist.h"

#include "mru.h"


MRUList* mru_new (guint max)
{
	MRUList *mru = malloc(sizeof(MRUList));
	mru->max = max;
	mru->list = g_elist_new();
	return mru;
}

MRUList* mru_new_from_list (guint max, GEList *list)
{
	MRUList *mru = malloc(sizeof(MRUList));
	mru->max = max;
	mru->list = list;
	return mru;
}


void mru_free (MRUList *mru)
{
	g_elist_free_data(mru->list);
	free(mru);
}


void mru_add (MRUList *mru, const gchar *entry)
{
	GList *found = g_list_find_custom(GLIST(mru->list), entry, (GCompareFunc)strcmp);

	if (found) {
		g_elist_remove_link(mru->list, found);
		g_elist_prepend(mru->list, found->data);
		g_list_free_1(found);
	} else {
		g_elist_prepend(mru->list, strdup(entry));
		if (g_elist_length(mru->list) > mru->max) {
			GList *last = g_elist_last(mru->list);
			g_elist_remove_link(mru->list, last);
			free(last->data);
			g_list_free_1(last);
		}
	}
}
