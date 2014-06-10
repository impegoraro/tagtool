#include <gtk/gtk.h>
#include <glade/glade.h>

#include "cursor.h"


static GtkWidget *w_main = NULL;


static void set_cursor(GtkWidget *widget, GdkCursor *cursor)
{
	gdk_window_set_cursor(widget->window, cursor);

	/* process events or we won't see the change */
	while (gtk_events_pending()) gtk_main_iteration();
}


void cursor_init(GladeXML *xml)
{
	w_main = GTK_WIDGET(glade_xml_get_widget(xml, "w_main"));
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

