#include <stdio.h>
#include <glib.h>
#include "elist.h"


GEList*	g_elist_new()
{
	GEList *list = g_malloc(sizeof(GEList));
	list->last = NULL;
	list->first = NULL;
	list->length = 0;

	return list;
}

void g_elist_free (GEList *list)
{
	g_list_free(list->first);
	g_free(list);
}

void g_elist_free_data (GEList *list)
{
	GList *next;
	GList *p = list->first;

	while (p) {
		next = p->next;
		g_free(p->data);
		g_list_free_1(p);
		p = next;
	}
	g_free(list);
}


void g_elist_append (GEList *list, gpointer data)
{
	GList *new_node = g_list_alloc();
	new_node->data = data;
	new_node->next = NULL;
	new_node->prev = list->last;

	if (list->last != NULL)
		list->last->next = new_node;
	list->last = new_node;
	if (list->first == NULL)
		list->first = new_node;
	list->length++;
}

void g_elist_prepend (GEList *list, gpointer data)
{
	list->first = g_list_prepend(list->first, data);
	if (list->last == NULL)
		list->last = list->first;
	list->length++;
}

void g_elist_insert (GEList *list, gpointer data, gint position)
{
	if ((position < 0) || (position >= list->length))
		g_elist_append(list, data);
	else {
		list->first = g_list_insert(list->first, data, position);
		if (list->last == NULL)
			list->last = list->first;
		list->length++;
	}
}

void g_elist_insert_sorted (GEList *list, gpointer data, GCompareFunc func)
{
	list->first = g_list_insert_sorted(list->first, data, func);
	if (list->last == NULL)
		list->last = list->first;
	else if (list->last->next != NULL)
		list->last = list->last->next;
	list->length++;
}


void g_elist_clear (GEList *list)
{
	g_list_free(list->first);
	list->first = NULL;
	list->last = NULL;
	list->length = 0;
}


void g_elist_remove (GEList *list, gpointer data)
{
	GList *llink = g_list_find(list->first, data);

	if (llink != NULL) {
		g_elist_remove_link(list, llink);
		g_list_free_1(llink);
	}
}

void g_elist_remove_link (GEList *list, GList *llink)
{
	GList *aux = list->first;

	if (llink == list->first)
		list->first = list->first->next;
	if (llink == list->last)
		list->last = list->last->prev;

	g_list_remove_link(aux, llink);

	list->length--;
}


gpointer g_elist_extract (GEList *list, GList *llink)
{
	gpointer result = llink->data;

	g_elist_remove_link(list, llink);
	g_list_free_1(llink);

	return result;
}


GEList* g_elist_concat (GEList *list1, GEList *list2)
{
	if (list2->length > 0) {
		if (list1->length == 0) {
			list1->first = list2->first;
		} else {
			list1->last->next = list2->first;
			list2->first->prev = list1->last;
		}			
		list1->last = list2->last;
		list1->length += list2->length;
	}
	g_free(list2);

	return list1;
}

GEList* g_elist_copy (GEList *list)
{
	GEList *new_list = g_elist_new();
	GList *new_node, *last_node, *src_node;

	src_node = list->first;
	last_node = NULL;
	new_node = NULL;
	while (src_node) {
		new_node = g_list_alloc();
		new_node->data = src_node->data;
		new_node->next = NULL;
		new_node->prev = last_node;

		if (last_node != NULL)
			last_node->next = new_node;
		else
			new_list->first = new_node;

		last_node = new_node;
		src_node = src_node->next;
	}
	new_list->last = new_node;
	new_list->length = list->length;

	return new_list;
}


void g_elist_reverse (GEList *list)
{
	GList *aux;

	aux = list->first;
	list->first = g_list_reverse(list->first);
	list->last = aux;
}

void g_elist_sort (GEList *list, GCompareFunc compare_func)
{
	GList *aux = list->last;

	list->first = g_list_sort(list->first, compare_func);
	list->last = g_list_last(aux); /* wish we could avoid this... */
}


/* 
 * the following exist mostly for debugging purposes 
 */

void g_elist_print(GEList *list) 
{
	GList *iter;
	for (iter = g_elist_first(list); iter; iter = iter->next)
		printf("0x%08X\n", (unsigned int)iter->data);
}

void g_elist_print_str(GEList *list) 
{
	GList *iter;
	for (iter = g_elist_first(list); iter; iter = iter->next)
		printf("%s\n", (char *)iter->data);
}

