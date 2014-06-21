#include <stdio.h>
#include <gtk/gtk.h>

#include "scan_progress_dlg.h"


/* widgets */
static GtkRevealer *revealer_scan_progress = NULL;
static GtkLabel *lab_scan_progress = NULL;
static GtkSpinner *scanningSpinner = NULL;
/* private data */
static gboolean stop_requested = FALSE;


/* UI callbacks */
void cb_dlg_scan_stop(GtkButton *button, gpointer user_data)
{
	stop_requested = TRUE;
}


/* public functions */
void spd_init(GtkBuilder *builder)
{
	/* widgets and icons */
	revealer_scan_progress = GTK_REVEALER(gtk_builder_get_object(builder, "revealer_scan_progress"));
	lab_scan_progress = GTK_LABEL(gtk_builder_get_object(builder, "lab_scan_progress"));
	scanningSpinner = GTK_SPINNER(gtk_builder_get_object(builder, "scanningSpinner"));
}

void spd_display()
{
	gtk_spinner_start(scanningSpinner);
	gtk_label_set_text(lab_scan_progress, "0\n0");
	gtk_revealer_set_reveal_child(revealer_scan_progress, TRUE);
}

void spd_hide()
{
	stop_requested = FALSE;
	gtk_revealer_set_reveal_child(revealer_scan_progress, FALSE);
	gtk_spinner_stop(scanningSpinner);
}

void spd_update(int dirs, int files)
{
	char buf[20];
	snprintf(buf, sizeof(buf), "%i\n%i", dirs, files);
	gtk_label_set_text(lab_scan_progress, buf);
}

gboolean spd_stop_requested()
{
	return stop_requested;
}

