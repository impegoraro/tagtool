#ifndef MRU_H
#define MRU_H


typedef struct {
  GEList *list;
  guint max;
} MRUList;


MRUList* mru_new (guint max);
MRUList* mru_new_from_list (guint max, GEList *list);
void mru_free (MRUList *mru);

void mru_add (MRUList *mru, const gchar *entry);



#endif

