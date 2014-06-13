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
	GdkPixbuf *logo;
	GError *error = NULL;

	dlg_about = GTK_WINDOW(gtk_about_dialog_new());
	char buffer[200];
	gchar *authors[3] = {
		"Pedro Ávila Lopes <paol1976@yahoo.com>",
		"Ilan Moreira Pegoraro <iemoreirap@gmail.com>",
		NULL
	};
	gchar *translators[17] = {
		"Rafael Bermúdez (es) <rafaelber@yahoo.com.mx>",
		"Christian Bjelle (sv) <christian@bjelle.se>",
		"Dovydas <laisve (lt)@gmail.com>",
		"Daniel van (nl) Eeden <daniel_e@dds.nl>",
		"Rafal Glazar (pl) <rafal_glazar@o2.pl>",
		"Dmytro Goykolov (ua) <goykolov@pisem.net>",
		"Jeremie Knuesel (fr) <jeremie.knuesel@epfl.ch>",
		"Oleksandr Korneta (ua) <atenrok@ua.fm>",
		"Vítězslav Kotrla (cs) <vitko@post.cz>",
		"Pedro Ávila (pt) Lopes <paol1976@yahoo.com>",
		"Pavel Maryanov (ru) <acid_jack@ukr.net>",
		"Christopher Orr (en_GB) <chris@protactin.co.uk>",
		"Rostislav Raykov (bg) <zbrox@i-space.org>",
		"Emilio Scalise (it) <emisca@rocketmail.com>",
		"Lucas Mazzardo (pt_BR) Veloso <lmveloso@gmail.com>",
		"Jan Wenzel (de) <scripter01@gmx.net>",
		NULL
	};
	
	/* set the title */
	gtk_window_set_title(dlg_about, _("About Audio Tag Tool"));

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg_about), PACKAGE_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg_about), PACKAGE_VERSION);
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dlg_about), GTK_LICENSE_GPL_2_0);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg_about), "http://pwp.netcabo.pt/paol/tagtool/");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg_about), (const gchar **)authors);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dlg_about),
					_("This software is made available under the GNU General Public Licence." \
					"See the file COPYING for the full license terms."));
	
	
	/* set the team of translators */
	//gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dlg_about), "Translators");
	gtk_about_dialog_add_credit_section (GTK_ABOUT_DIALOG(dlg_about),
                                     _("Translated by:"),
                                     (const gchar**)translators);

	logo = gdk_pixbuf_new_from_file(DATADIR"/TagTool.png", &error);
	if(logo != NULL) 
		gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dlg_about), logo);
	else {
		printf("Error: unable to logo: %s", error->message);
		g_clear_error(&error);
	}

	// /* set the supported audio formats */
	strcpy(buffer, _("Easily manage the tags of your music files.\n Supported audio formats: "));

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