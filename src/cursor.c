#include <gtk/gtk.h>

#include "cursor.h"


static GtkWidget *w_main = NULL;


static void set_cursor(GtkWidget *widget, GdkCursor *cursor)
{
	gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);

	/* process events or we won't see the change */
	while (gtk_events_pending()) gtk_main_iteration();
}


void cursor_init(GtkBuilder *builder)
{
	w_main = GTK_WIDGET(gtk_builder_get_object(builder, "w_main"));
}

void cursor_set_wait()
{
	static GdkCursor *cur = NULL;
	if (!cur)
		cur = gdk_cursor_new(GDK_WATCH);

	set_cursor(w_main, cur);
}

void cursor_set_normal()
{
	static GdkCursor *cur = NULL;
	if (!cur)
		cur = gdk_cursor_new(GDK_LEFT_PTR);

	set_cursor(w_main, cur);
}

