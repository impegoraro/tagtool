#include <stdarg.h>
#include <gtk/gtk.h>


int message_box(GtkWindow *transientfor, char *title, char* text, GtkButtonsType buttons, int defbutton)
{
	GtkDialog *dlg;
	int result;

	/* create the dialog */
	dlg = GTK_DIALOG(gtk_message_dialog_new(transientfor, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, buttons,text));
	
	gtk_window_set_title(GTK_WINDOW(dlg), title);
	gtk_dialog_set_default_response(dlg, defbutton);

	/* show the dialog and wait for it to finish */
	gtk_widget_show_all(GTK_WIDGET(dlg));
	result = gtk_dialog_run(dlg);

	/* cleanup and return the button pressed */
	gtk_widget_destroy(GTK_WIDGET(dlg));
	return result;
}

