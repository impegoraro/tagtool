#ifndef RENAME_DLG_H
#define RENAME_DLG_H

#include <glade/glade.h>


void rename_init(GladeXML *xml);

char *rename_prompt_new_name(const char *old_name);


#endif
