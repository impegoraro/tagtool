#include <stdio.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "status_bar.h"


#define MAX_MSG_SIZE  256


/* widgets */
static GtkStatusbar *statusbar = NULL;

/* private data */
static gchar statusbar_msg[MAX_MSG_SIZE];
static guint statusbar_ctx = 0;


void sb_init(GladeXML *xml)
{
	statusbar = GTK_STATUSBAR(glade_xml_get_widget(xml, "statusbar"));

	statusbar_ctx = gtk_statusbar_get_context_id(statusbar, "status_msg");
}


void sb_printf(const gchar *format, ...)
{
	va_list ap;

	va_start(ap, format);
        vsnprintf(statusbar_msg, MAX_MSG_SIZE, format, ap);
	va_end(ap);
	statusbar_msg[MAX_MSG_SIZE-1] = 0;

	gtk_statusbar_pop(statusbar, statusbar_ctx);
	gtk_statusbar_push(statusbar, statusbar_ctx, statusbar_msg);
}


void sb_clear()
{
	gtk_statusbar_pop(statusbar, statusbar_ctx);
}

