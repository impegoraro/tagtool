#ifndef EDIT_TAB_H
#define EDIT_TAB_H

#include "elist.h"


void et_init(GladeXML *xml);

void et_load_file(const gchar *name);

void et_unload_file();


#endif
