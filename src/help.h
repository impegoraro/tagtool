#ifndef HELP_H
#define HELP_H

enum {
	HELP_TAG_FORMAT = 0,
	HELP_RENAME_FORMAT = 1
};


void help_init();

void help_display(GtkWidget* parent, int topic);


#endif
