#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "scan_progress_dlg.h"


/* widgets */
static GtkWindow *dlg_scan_progress = NULL;
static GtkLabel *lab_scan_progress = NULL;

/* private data */
static gboolean stop_requested = FALSE;


/* UI callbacks */
void cb_dlg_scan_stop(GtkButton *button, gpointer user_data)
{
	stop_requested = TRUE;
}


/* public functions */
void spd_init(GladeXML *xml)
{
	/* widgets and icons */
	dlg_scan_progress = GTK_WINDOW(glade_xml_get_widget(xml, "dlg_scan_progress"));
	lab_scan_progress = GTK_LABEL(glade_xml_get_widget(xml, "lab_scan_progress"));

	gtk_window_set_transient_for(dlg_scan_progress, GTK_WINDOW(glade_xml_get_widget(xml, "w_main")));
}

void spd_display()
{
	gtk_label_set_text(lab_scan_progress, "0\n0");
	gtk_window_present(dlg_scan_progress);
}

void spd_hide()
{
	stop_requested = FALSE;
	gtk_widget_hide(GTK_WIDGET(dlg_scan_progress));
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

