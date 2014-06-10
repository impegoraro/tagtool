#ifndef VORBIS_EDIT_FIELD_H
#define VORBIS_EDIT_FIELD_H

#include "vorbis_file.h"


gboolean vorbis_editfld_create_comment(vorbis_file *file);

gboolean vorbis_editfld_edit_comment(vorbis_file *file, const char *name);

void vorbis_editfld_init(GladeXML *xml);


#endif
