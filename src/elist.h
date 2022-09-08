#ifndef ELIST_H
#define ELIST_H

#include <gmodule.h>
#include <sys/types.h>

/****************************************************************************
 *	Crude wrapper for glib lists (GList).
 *	Also adds macros that let you treat the list as a stack or a queue.
 ****************************************************************************/


typedef struct {
  GList *first;
  GList *last;
  size_t length;
} GEList;


#define  GLIST(list)		((list)->first)

GEList*	 g_elist_new		();				/* new */
void	 g_elist_free		(GEList		*list);
void	 g_elist_free_data	(GEList		*list);		/* new */

void	 g_elist_append		(GEList		*list,
				 gpointer	 data);
void	 g_elist_prepend	(GEList		*list,
				 gpointer	 data);
void	 g_elist_insert		(GEList		*list,
				 gpointer	 data,
				 size_t 	 position);
void	 g_elist_insert_sorted	(GEList		*list,
				 gpointer	 data,
				 GCompareFunc	 func);
void	 g_elist_clear		(GEList		*list);		/* new */
void	 g_elist_remove		(GEList		*list,
				 gpointer	 data);
void	 g_elist_remove_link	(GEList		*list,
				 GList		*llink);
gpointer g_elist_extract	(GEList		*list,		/* new */
				 GList		*llink);
GEList*	 g_elist_concat		(GEList		*list1,
				 GEList		*list2);
GEList*	 g_elist_copy		(GEList		*list);
void	 g_elist_reverse	(GEList		*list);
void	 g_elist_sort		(GEList		*list,
				 GCompareFunc	 compare_func);

#define  g_elist_push(l,d)	(g_elist_append(l,d))		/* new */
#define  g_elist_pop(l)		(g_elist_extract(l,(l)->last))	/* new */
#define  g_elist_enqueue(l,d)	(g_elist_append(l,d))		/* new */
#define  g_elist_dequeue(l)	(g_elist_extract(l,(l)->first))	/* new */

#define  g_elist_last(l)	((l)->last)
#define  g_elist_first(l)	((l)->first)
#define  g_elist_length(l)	((l)->length)


#endif
