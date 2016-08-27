#include <config.h>
#include <string.h>
#include <gtk/gtk.h>

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
static GtkWindow   *w_main        = NULL;
static GtkStack    *s_mainStack   = NULL;
static GtkPaned    *p_innerPaned  = NULL;
static GtkLabel    *l_version     = NULL;

/* prefs variables */
static int *width;
static int *height;
static int *panedPos;


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

void cb_innerPaned_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	*panedPos = gtk_paned_get_position(p_innerPaned);
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

void mw_init(GtkBuilder *builder)
{
	/* 
	 * Tabs are visible by default to make editing in glade easier.
	 * They must be hidden before showing the window, otherwise they 
	 * would affect the window size.
	 */
 	gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "w_no_file")));
 	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "box_tag_file")));
	
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gtk_builder_get_object(builder, "nb_edit")), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gtk_builder_get_object(builder, "nb_id3v1")), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gtk_builder_get_object(builder, "nb_id3v2")), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gtk_builder_get_object(builder, "nb_vorbis")), FALSE);

	// sets the image of the main notebook pages...
	//GtkImage *img_tab_edit = GTK_IMAGE(gtk_builder_get_object(builder, "img_tab_edit"));
	//gtk_image_set_from_file(img_tab_edit, DATADIR"/tab_edit.png");
	//GtkImage *img_tab_tag = GTK_IMAGE(gtk_builder_get_object(builder, "img_tab_tag"));
	//gtk_image_set_from_file(img_tab_tag, DATADIR"/tab_tag.png");
	//GtkImage *img_tab_clear = GTK_IMAGE(gtk_builder_get_object(builder, "img_tab_clear"));
	//gtk_image_set_from_file(img_tab_clear, DATADIR"/tab_clear.png");
	//GtkImage *img_tab_rename = GTK_IMAGE(gtk_builder_get_object(builder, "img_tab_rename"));
	//gtk_image_set_from_file(img_tab_rename, DATADIR"/tab_rename.png");
	//GtkImage *img_tab_playlist = GTK_IMAGE(gtk_builder_get_object(builder, "img_tab_playlist"));
	//gtk_image_set_from_file(img_tab_playlist, DATADIR"/tab_playlist.png");

	/*
	 * Hide the MP3 and Vorbis menus.  This has to be done here because
	 * the MP3/Vorbis specific code may not be compiled.
	 */
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "m_id3")));
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "m_vorbis")));


	/*
	 * Get the widgets from glade and setup the interface
	 */
	w_main = GTK_WINDOW(gtk_builder_get_object(builder, "w_main"));
	s_mainStack = GTK_STACK(gtk_builder_get_object(builder, "s_mainStack"));
  	p_innerPaned = GTK_PANED(gtk_builder_get_object(builder, "p_innerPaned"));
	l_version = GTK_LABEL(gtk_builder_get_object(builder, "l_version"));

  	gtk_window_set_titlebar(w_main, GTK_WIDGET(gtk_builder_get_object(builder, "b_mainTitle")));
	{
	  	char versionStr[100];
	  	sprintf(versionStr, "<span size=\"small\">%s</span>", PACKAGE_VERSION);
	  	gtk_label_set_markup(l_version, versionStr);
	}
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

  	/* Panel position */
  	panedPos = pref_get_ref("mw:panedPos");
  	if(!panedPos) {
	  	guint temp = 120;
		panedPos = pref_set("mw:panedPos", PREF_INT, &temp);
	}

	/*
	 * initialize the main window
	 */
	if (*width && *height)
		gtk_window_set_default_size(w_main, *width, *height);
  	if (*panedPos)
		gtk_paned_set_position(p_innerPaned, *panedPos);

	gtk_widget_show(GTK_WIDGET(w_main));
}

