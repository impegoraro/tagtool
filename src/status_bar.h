#ifndef STATUS_BAR_H
#define STATUS_BAR_H


void sb_init(GtkBuilder *builder);

void sb_printf(const gchar *format, ...);

void sb_clear();


#endif
