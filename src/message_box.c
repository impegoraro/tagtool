#include <stdarg.h>
#include <gtk/gtk.h>


int message_box(GtkWindow *transientfor, char *title, char* text, int defbutton, ...)
{
	va_list ap;
	GtkDialog *dlg;
	GtkWidget *label;
	char *button_label;
	int i;
	int result;

	/* create the dialog */
	dlg = GTK_DIALOG(gtk_dialog_new());
	gtk_window_set_title(GTK_WINDOW(dlg), title);
	gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
	if (transientfor != NULL)
		gtk_window_set_transient_for(GTK_WINDOW(dlg), transientfor);

	label = gtk_label_new(text);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_padding(GTK_MISC(label), 25, 25);
	gtk_container_add(GTK_CONTAINER(dlg->vbox), label);

	/* add the buttons */
	va_start(ap, defbutton);
	i = 0;
	while ( (button_label = va_arg(ap, char *)) ) {
		gtk_dialog_add_button(dlg, button_label, i);
		i++;
	}
	va_end(ap);
	gtk_dialog_set_default_response(dlg, defbutton);

	/* show the dialog and wait for it to finish */
	gtk_widget_show_all(GTK_WIDGET(dlg));
	result = gtk_dialog_run(dlg);

	/* cleanup and return the button pressed */
	gtk_widget_destroy(GTK_WIDGET(dlg));
	return result;
}

