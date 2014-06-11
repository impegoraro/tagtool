#include <config.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "about.h"


/* private data */
static GtkWindow *dlg_about = NULL;

/* UI callbacks */

void cb_about_close(GtkButton *button, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(dlg_about));
}


/* public functions */

void about_display()
{
	//gtk_widget_show(GTK_WIDGET(dlg_about));
	//gtk_show_about_dialog(GTK_ABOUT_DIALOG(dlg_about), NULL);
	gtk_widget_show(GTK_WIDGET(dlg_about));
}

void about_init(GtkBuilder *builder)
{
	dlg_about = GTK_WINDOW(gtk_about_dialog_new());
	char buffer[200];
	gchar *authors[3] = {
		"Pedro Ávila Lopes       <paol1976@yahoo.com>",
		"Ilan Moreira Pegoraro   <iemoreirap@gmail.com>",
		NULL
	};

	
	//gtk_window_set_transient_for(dlg_about, w_main);

	// /* set the icon */
	//gtk_about_dialog_set_logo(dlg_about, DATADIR"/TagTool.png");

	// /* set the title */
	gtk_window_set_title(dlg_about, _("About Audio Tag Tool"));

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg_about), PACKAGE_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg_about), PACKAGE_VERSION);
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dlg_about), GTK_LICENSE_GPL_2_0);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg_about), "http://pwp.netcabo.pt/paol/tagtool/");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg_about), (const gchar **)authors);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dlg_about),
					"This software is made available under the GNU General Public Licence." \
					"See the file COPYING for the full license terms.");
	// /* set the supported audio formats */
	strcpy(buffer, _("easily manage the tags on your music files.\n Supported audio formats: "));

#ifdef ENABLE_MP3
	strcat(buffer, "MPEG");
#endif

	if (buffer[0] != 0)
		strcat(buffer, ", ");

#ifdef ENABLE_VORBIS
	strcat(buffer, "Ogg Vorbis");
#endif

	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dlg_about), buffer);
}