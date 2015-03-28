#ifndef MPLAYER_H_
#define MPLAYER_H_

#include <gtk/gtk.h>

void mp_init(GtkBuilder *builder);
void mp_add_track(const gchar* trackPath);
void mp_stop();

#endif