#include <config.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "about.h"


/* private data */
static GtkWindow *dlg_about = NULL;
static GtkWindow *w_main = NULL;

/* UI callbacks */

void cb_about_close(GtkButton *button, gpointer user_data)
{
  (void)button; (void)user_data;
  gtk_widget_hide(GTK_WIDGET(dlg_about));
}


/* public functions */

void about_display()
{
	GdkPixbuf *logo;
	GError *error = NULL;
	char buffer[200];
	const gchar *authors[3] = {
		"Pedro Ávila Lopes <paol1976@yahoo.com>",
		"Ilan Moreira Pegoraro <iemoreirap@gmail.com>",
		NULL
	};

	const gchar* copyright = "Copyright \xc2\xa9 2008 Pedro Lopes.\n" \
                           "Copyright \xc2\xa9 2013 Ilan Pegoraro.\n";

	const gchar *translators = 
		"Rafael Bermúdez (es) <rafaelber@yahoo.com.mx>\n" \
		"Christian Bjelle (sv) <christian@bjelle.se>\n" \
		"Dovydas <laisve (lt)@gmail.com>\n" \
		"Daniel van (nl) Eeden <daniel_e@dds.nl>\n" \
		"Rafal Glazar (pl) <rafal_glazar@o2.pl>\n" \
		"Dmytro Goykolov (ua) <goykolov@pisem.net>\n" \
		"Jeremie Knuesel (fr) <jeremie.knuesel@epfl.ch>\n" \
		"Oleksandr Korneta (ua) <atenrok@ua.fm>\n" \
		"Vítězslav Kotrla (cs) <vitko@post.cz>\n" \
		"Pedro Ávila (pt) Lopes <paol1976@yahoo.com>\n" \
		"Pavel Maryanov (ru) <acid_jack@ukr.net>\n" \
		"Christopher Orr (en_GB) <chris@protactin.co.uk>\n" \
		"Rostislav Raykov (bg) <zbrox@i-space.org>\n" \
		"Emilio Scalise (it) <emisca@rocketmail.com>\n" \
		"Lucas Mazzardo Veloso (pt_BR) <lmveloso@gmail.com>\n" \
		"Jan Wenzel (de) <scripter01@gmx.net>\n";
	
	gchar *license = _("This software is made available under the GNU General Public Licence." \
					"See the file COPYING for the full license terms.");
	
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

	logo = gdk_pixbuf_new_from_file(DATADIR"/TagTool.png", &error);
	if(!logo) {
		printf("Error: unable to logo: %s", error->message);
		g_clear_error(&error);
	}
	gtk_show_about_dialog(w_main, "authors", (const gchar **)authors,
			"program-name", PACKAGE_NAME,
			"version", PACKAGE_VERSION,
			"logo", logo,
			"website", "https://github.com/impegoraro/tagtool/wiki",
			"comments", buffer,
			"license-type", GTK_LICENSE_GPL_2_0,
			"license", license,
			"translator-credits", translators, 
			"copyright", copyright,
			NULL);
}

void about_init(GtkBuilder *builder)
{
  	w_main = GTK_WINDOW(gtk_builder_get_object(builder, "w_main"));
}
