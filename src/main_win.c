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
static GtkComboBox *combo_wd      = NULL;
static GtkBox      *b_file_list   = NULL;
static GtkBox      *box_header_left = NULL;

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
  g_object_set(G_OBJECT(box_header_left), "width-request", gtk_widget_get_allocated_width(GTK_WIDGET(b_file_list)) + 10 , NULL);
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
  combo_wd = GTK_COMBO_BOX(gtk_builder_get_object(builder, "combo_wd"));
  b_file_list = GTK_BOX(gtk_builder_get_object(builder, "b_file_list"));
  box_header_left = GTK_BOX(gtk_builder_get_object(builder, "box_header_left"));

	{
	  	char versionStr[100];
	  	sprintf(versionStr, "<span size=\"small\">v%s</span>", PACKAGE_VERSION);
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

  /* Load css if exists */
  GError *error = NULL;
  GtkCssProvider *themeProvider = gtk_css_provider_new();
  GdkDisplay *display = gdk_display_get_default();
  GdkScreen *screen = gdk_display_get_default_screen (display);
  if (!gtk_css_provider_load_from_path(themeProvider, DATADIR "/basic.css", &error) && error != NULL) {
    g_print("Error loading theme: %s\n", error->message);
    g_error_free(error);
  }

  gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER(themeProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

	gtk_widget_show(GTK_WIDGET(w_main));
}

