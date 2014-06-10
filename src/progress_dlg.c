#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "prefs.h"
#include "progress_dlg.h"


#define MAX_MSG_SIZE  512


/* widgets */
static GtkWidget *w_main = NULL;
static GtkWindow *dlg_progress = NULL;
static GtkWidget *b_stop = NULL;
static GtkWidget *b_close = NULL;
static GtkTreeView *tv_progress = NULL;
static GtkListStore *store_progress = NULL;

/* icons */
static GdkPixbuf *pix_table[4];

/* prefs */
static int *width;
static int *height;

/* private data */
static gboolean stop_requested = FALSE;


/* private functions */
static void tree_view_setup()
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* model */
	store_progress = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model(tv_progress, GTK_TREE_MODEL(store_progress));

	/* columns and renderers */
	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(tv_progress, col);

	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(renderer, "xpad", 4, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", 0);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ypad", 0, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);

	/* disable selection */
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tv_progress),
				    GTK_SELECTION_NONE);
}


/* UI callbacks */
void cb_dlg_progress_stop(GtkButton *button, gpointer user_data)
{
	stop_requested = TRUE;
}

void cb_dlg_progress_size_changed(GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
	*width = alloc->width;
	*height = alloc->height;
}


/* public functions */
void pd_init(GladeXML *xml)
{
	gint temp = 0;

	/* widgets and icons */
	w_main = glade_xml_get_widget(xml, "w_main");
	b_stop = glade_xml_get_widget(xml, "b_stop");
	b_close = glade_xml_get_widget(xml, "b_close");
	dlg_progress = GTK_WINDOW(glade_xml_get_widget(xml, "dlg_progress"));
	tv_progress = GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_progress"));

	pix_table[PD_ICON_INFO] = gdk_pixbuf_new_from_file(DATADIR"/info.png", NULL);
	pix_table[PD_ICON_OK]   = gdk_pixbuf_new_from_file(DATADIR"/ok.png", NULL);
	pix_table[PD_ICON_FAIL] = gdk_pixbuf_new_from_file(DATADIR"/fail.png", NULL);
	pix_table[PD_ICON_WARN] = gdk_pixbuf_new_from_file(DATADIR"/warn.png", NULL);

	/* preferences */
	width = pref_get_ref("prog:width");
	height = pref_get_ref("prog:height");
	if (width == NULL)
		width = pref_set("prog:width", PREF_INT, &temp);
	if (height == NULL)
		height = pref_set("prog:height", PREF_INT, &temp);

	if (*width && *height)
		gtk_window_set_default_size(dlg_progress, *width, *height);

	/* set up the tree view */
	tree_view_setup();
}

void pd_start(const char *title)
{
	stop_requested = FALSE;

	gtk_window_set_title(dlg_progress, title ? title : _("Progress"));
	gtk_widget_show(b_stop);
	gtk_widget_hide(b_close);
	gtk_list_store_clear(store_progress);

	gtk_window_present(dlg_progress);
}

void pd_end()
{
	stop_requested = FALSE;

	pd_scroll_to_bottom();
	gtk_widget_hide(b_stop);
	gtk_widget_show(b_close);
}

gboolean pd_stop_requested()
{
	return stop_requested;
}

void pd_printf(int icon, const char *format, ...)
{
	va_list ap;
	char text[MAX_MSG_SIZE];
	GtkTreeIter iter;

	va_start(ap, format);
	vsnprintf(text, MAX_MSG_SIZE, format, ap);
	va_end(ap);
	text[MAX_MSG_SIZE-1] = 0;

	gtk_list_store_append(store_progress, &iter);
	gtk_list_store_set(store_progress, &iter, 
			   0, (icon == PD_ICON_NONE ? NULL : pix_table[icon]),
			   1, text,
			   -1);
}

void pd_scroll_to_bottom()
{
	GtkAdjustment *adj;

	adj = gtk_tree_view_get_vadjustment(tv_progress);
	gtk_adjustment_set_value(adj, adj->upper);
}

