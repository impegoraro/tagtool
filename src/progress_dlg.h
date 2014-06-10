#ifndef PROGRESS_DLG_H
#define PROGRESS_DLG_H


#define PD_ICON_INFO  0
#define PD_ICON_OK    1
#define PD_ICON_FAIL  2
#define PD_ICON_WARN  3
#define PD_ICON_NONE  4


void pd_init(GladeXML *xml);

void pd_start(const char *title);

void pd_end();

gboolean pd_stop_requested();

void pd_printf(int icon, const char *format, ...);

void pd_scroll_to_bottom();


#endif
