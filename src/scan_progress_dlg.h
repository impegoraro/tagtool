#ifndef SCAN_PROGRESS_DLG_H
#define SCAN_PROGRESS_DLG_H


void spd_init(GladeXML *xml);

void spd_display();

void spd_hide();

void spd_update(int dirs, int files);

gboolean spd_stop_requested();


#endif
