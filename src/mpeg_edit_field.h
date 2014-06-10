#ifndef MPEG_EDIT_FIELD_H
#define MPEG_EDIT_FIELD_H

#include <id3.h>
#include "mpeg_file.h"


gboolean mpeg_editfld_create_frame(mpeg_file *file, int tag_version);

gboolean mpeg_editfld_edit_frame(mpeg_file *file, int tag_version, ID3Frame *frame);

void mpeg_editfld_init(GladeXML *xml);


#endif
