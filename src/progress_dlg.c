#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "prefs.h"
#include "progress_dlg.h"


#define MAX_MSG_SIZE  512


/* widgets */
static GtkWidget *w_main = NULL;
static GtkWidget *b_stop = NULL;
static GtkPaned  *p_mainPaned = NULL;
static GtkTreeView *tv_progress = NULL;
static GtkListStore *store_progress = NULL;

/* icons */
static GdkPixbuf *pix_table[4];

/* prefs */
static int *panedPos;

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

void cb_main_paned_resize(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	*panedPos = gtk_paned_get_position(p_mainPaned);
}


/* public functions */
void pd_init(GtkBuilder *builder)
{
	gint temp = 100;

	/* widgets and icons */
	w_main = GTK_WIDGET(gtk_builder_get_object(builder, "w_main"));
	b_stop = GTK_WIDGET(gtk_builder_get_object(builder, "b_stop"));
	p_mainPaned = GTK_PANED(gtk_builder_get_object(builder, "p_mainPaned"));
	tv_progress = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tv_progress"));

	pix_table[PD_ICON_INFO] = gdk_pixbuf_new_from_file(DATADIR"/info.png", NULL);
	pix_table[PD_ICON_OK]   = gdk_pixbuf_new_from_file(DATADIR"/ok.png", NULL);
	pix_table[PD_ICON_FAIL] = gdk_pixbuf_new_from_file(DATADIR"/fail.png", NULL);
	pix_table[PD_ICON_WARN] = gdk_pixbuf_new_from_file(DATADIR"/warn.png", NULL);

	/* preferences */
	panedPos = pref_get_ref("prog:panedPos");
	if (!panedPos)
		panedPos = pref_set("prog:panedPos", PREF_INT, &temp);
	if (*panedPos)
		gtk_paned_set_position(p_mainPaned, *panedPos);

  	gtk_widget_hide(b_stop);
	/* set up the tree view */
	tree_view_setup();
}

void pd_start(const char *title)
{
	stop_requested = FALSE;

	gtk_widget_show(b_stop);
	gtk_list_store_clear(store_progress);
}

void pd_end()
{
	stop_requested = FALSE;

	pd_scroll_to_bottom();
	gtk_widget_hide(b_stop);
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

	adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(tv_progress));
	gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
}

