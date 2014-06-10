#include <string.h>
#include <gtk/gtk.h>
#include "rename_dlg.h"


/* widgets */
static GtkDialog *dlg_rename_file = NULL;
static GtkEntry *ent_file_name = NULL;


char *rename_prompt_new_name(const char *old_name)
{
	char *new_name = NULL;
	int dlg_result;

	gtk_entry_set_text(ent_file_name, old_name);
	gtk_editable_select_region(GTK_EDITABLE(ent_file_name), 0, -1);
	gtk_widget_grab_focus(GTK_WIDGET(ent_file_name));

	gtk_window_present(GTK_WINDOW(dlg_rename_file));

	dlg_result = gtk_dialog_run(dlg_rename_file);
	if (dlg_result == GTK_RESPONSE_OK) {
		new_name = (char *)gtk_entry_get_text(ent_file_name);

		//printf("1: (%p) %s\n", new_name, new_name);

		if (strcmp(old_name, new_name) == 0)
			new_name = NULL;
		else
			new_name = g_strdup(new_name);
	}

	gtk_widget_hide(GTK_WIDGET(dlg_rename_file));

	return new_name;
}


void rename_init(GladeXML *xml)
{
	dlg_rename_file = GTK_DIALOG(glade_xml_get_widget(xml, "dlg_rename_file"));
	ent_file_name = GTK_ENTRY(glade_xml_get_widget(xml, "ent_file_name"));

}


