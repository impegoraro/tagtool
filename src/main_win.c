#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "str_util.h"
#include "str_convert.h"
#include "file_util.h"
#include "gtk_util.h"
#include "prefs.h"
#include "about.h"
#include "prefs_dlg.h"
#include "char_conv_dlg.h"
#include "file_list.h"
#include "edit_tab.h"
#include "main_win.h"


enum {
	TAB_EDIT = 0,
	TAB_TAG,
	TAB_CLEAR,
	TAB_RENAME,
	TAB_PLAYLIST
};


/* widgets */
static GtkWindow *w_main = NULL;
static GtkNotebook *nb_main = NULL;

/* prefs variables */
static int *width;
static int *height;


/*** private functions ******************************************************/



/*** UI callbacks ***********************************************************/

/* menu callbacks */
void cb_file_refresh(GtkWidget *widget, GdkEvent *event)
{
	fl_refresh(TRUE);
}

void cb_file_quit(GtkWidget *widget, GdkEvent *event)
{
	gtk_main_quit();
}

void cb_settings_charconv(GtkWidget *widget, GdkEvent *event)
{
	chconv_display(CHCONV_TAG);
}

void cb_settings_id3prefs(GtkWidget *widget, GdkEvent *event)
{
	prefs_display();
}

void cb_help_contents(GtkWidget *widget, GdkEvent *event)
{
/*
	GError *err = NULL;        

	// XXX - should check with g_find_program_in_path()

	g_spawn_command_line_async ("yelp "HELPDIR"tagtool.xml", &err);
*/
}

void cb_help_about(GtkWidget *widget, GdkEvent *event)
{
	about_display();
}

/* other UI callbacks */
gboolean cb_main_win_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	/* this will prompt to save changes, if necessary */
	et_unload_file();
	return FALSE;
}

void cb_main_win_size_changed(GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
	*width = alloc->width;
	*height = alloc->height;
}


/*** public functions *******************************************************/

void mw_init(GladeXML *xml)
{
	/* 
	 * Tabs are visible by default to make editing in glade easier.
	 * They must be hidden before showing the window, otherwise they 
	 * would affect the window size.
	 */
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_file")), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_edit")), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_id3v1")), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_id3v2")), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_vorbis")), FALSE);

	/* 
	 * Hide the MP3 and Vorbis menus.  This has to be done here because 
	 * the MP3/Vorbis specific code may not be compiled.
	 */
	gtk_widget_hide(GTK_WIDGET(glade_xml_get_widget(xml, "m_id3")));
	gtk_widget_hide(GTK_WIDGET(glade_xml_get_widget(xml, "m_vorbis")));


	/*
	 * Get the widgets from glade and setup the interface
	 */
	w_main = GTK_WINDOW(glade_xml_get_widget(xml, "w_main"));
	nb_main = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_main"));


	/*
	 * get the preference values, or set them to defaults
	 */

	/* width */
	width = pref_get_ref("mw:width");
	if (!width) {
		gint temp = 0;
		width = pref_set("mw:width", PREF_INT, &temp);
	}

	/* height */
	height = pref_get_ref("mw:height");
	if (!height) {
		gint temp = 0;
		height = pref_set("mw:height", PREF_INT, &temp);
	}


	/*
	 * initialize the main window
	 */
	if (*width && *height)
		gtk_window_set_default_size(w_main, *width, *height);
	gtk_widget_show(GTK_WIDGET(w_main));
}

