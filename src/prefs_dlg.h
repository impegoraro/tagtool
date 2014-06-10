#ifndef PREFS_DLG_H
#define PREFS_DLG_H

#include <glade/glade.h>


enum {
	WRITE_AUTO = 0,
	WRITE_V1 = 0x1,
	WRITE_V2 = 0x2,
};


typedef struct {
	int write_id3_version;
	gboolean preserve_existing;
} id3_prefs;


id3_prefs prefs_get_id3_prefs();

void prefs_display();

void prefs_init(GladeXML *xml);


#endif
