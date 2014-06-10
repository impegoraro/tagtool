#include <config.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "about.h"


/* private data */
static GtkWindow *dlg_about = NULL;
static GtkWindow *dlg_credits = NULL;


/* UI callbacks */

void cb_about_credits(GtkButton *button, gpointer user_data)
{
	gtk_widget_show(GTK_WIDGET(dlg_credits));
}

void cb_about_close(GtkButton *button, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(dlg_credits));
	gtk_widget_hide(GTK_WIDGET(dlg_about));
}


/* public functions */

void about_display()
{
	gtk_widget_show(GTK_WIDGET(dlg_about));
}

void about_init(GladeXML *xml)
{
	GtkWindow *w_main = NULL;
	GtkLabel *lab_about_version = NULL;
	GtkLabel *lab_about_supported = NULL;
	GtkImage *img_about = NULL;
	char buffer[100];

	dlg_about = GTK_WINDOW(glade_xml_get_widget(xml, "dlg_about"));
	dlg_credits = GTK_WINDOW(glade_xml_get_widget(xml, "dlg_credits"));
	w_main = GTK_WINDOW(glade_xml_get_widget(xml, "w_main"));
	lab_about_version = GTK_LABEL(glade_xml_get_widget(xml, "lab_about_version"));
	lab_about_supported = GTK_LABEL(glade_xml_get_widget(xml, "lab_about_supported"));
	img_about = GTK_IMAGE(glade_xml_get_widget(xml, "img_about"));
	
	gtk_window_set_transient_for(dlg_about, w_main);
	gtk_window_set_transient_for(dlg_credits, dlg_about);

	/* set the icon */
	gtk_image_set_from_file(img_about, DATADIR"/TagTool.png");

	/* set the title */
	snprintf(buffer, sizeof(buffer), gtk_label_get_label(lab_about_version), PACKAGE_VERSION);
	gtk_label_set_markup(lab_about_version, buffer);

	/* set the supported audio formats */
	buffer[0] = 0;

#ifdef ENABLE_MP3
	strcat(buffer, "MPEG");
#endif

	if (buffer[0] != 0)
		strcat(buffer, ", ");

#ifdef ENABLE_VORBIS
	strcat(buffer, "Ogg Vorbis");
#endif

	gtk_label_set_text(lab_about_supported, buffer);
	
}

