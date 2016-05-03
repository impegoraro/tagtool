#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtk_util.h"
#include "elist.h"
#include "str_util.h"
#include "vorbis_edit_field.h"


enum {
	MODE_CREATE,
	MODE_EDIT
};


/* widgets */
static GtkDialog *dlg;
static GtkGrid *grid;
static GtkEntry *entry;
static GtkButton *b_ok;
static GtkButton *b_cancel;
static GtkWidget *widget = NULL;  /* dinamically created "field" widget */


/*** private functions ******************************************************/
gboolean cb_key_press (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  	if(((GdkEventKey*)event)->keyval == GDK_KEY_KP_Enter || ((GdkEventKey*)event)->keyval == GDK_KEY_Return) {
	  	gtk_dialog_response(dlg, GTK_RESPONSE_OK);
	} else if(((GdkEventKey*)event)->keyval == GDK_KEY_Escape) {
  		gtk_dialog_response(dlg, GTK_RESPONSE_CANCEL);
	  	return TRUE;
	}
  	return FALSE;
}

static void fill_names_combo(GtkComboBox *combo)
{
	const char **name_array;
	GEList *name_list;
	int count;
	int i;

	name_list = g_elist_new();
	count = vorbis_file_get_std_fields(&name_array);
	for (i = 0; i < count; i++)
		g_elist_append(name_list, (void*)name_array[i]);

	g_elist_sort(name_list, (GCompareFunc)strcoll);
	
	g_list_foreach(GLIST(name_list), glist_2_combo, combo);
	g_elist_free(name_list);
}


static void set_ui(int mode, vorbis_file *file, const char *name)
{
	const char *title;
	const char *value;

  	if(widget == NULL) {
		widget = gtk_combo_box_text_new_with_entry();
		fill_names_combo(GTK_COMBO_BOX(widget));
	  	gtk_grid_attach(grid, widget, 1, 0, 1, 1);

		gtk_dialog_set_default_response(dlg, GTK_RESPONSE_OK);

		gtk_widget_unparent(GTK_WIDGET(b_cancel));
  		gtk_dialog_add_action_widget(dlg, GTK_WIDGET(b_cancel), GTK_RESPONSE_CANCEL);
	  	gtk_widget_unparent(GTK_WIDGET(b_ok));
  		gtk_dialog_add_action_widget(dlg, GTK_WIDGET(b_ok), GTK_RESPONSE_OK);
  	}

	if (mode == MODE_CREATE) {
		title = _("New Vorbis Field");
	}
	else {
		title = _("Edit Vorbis Field");

		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget))), name);

		vorbis_file_get_field_by_name(file, name, &value);
		gtk_entry_set_text(entry, value);
	}

	gtk_window_set_title(GTK_WINDOW(dlg), title);
	gtk_widget_show_all(GTK_WIDGET(dlg));
}

static void clear_ui()
{
	gtk_entry_set_text(entry, "");
	//gtk_widget_destroy(widget);
}


static void update_comments(int mode, vorbis_file *file, const char *orig_name)
{
	char *name = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(widget))), 0, -1);
	char *value = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

	str_ascii_tolower(name);

	if (mode == MODE_CREATE) {
		vorbis_file_append_field_by_name(file, name, value);
	}
	else {
		if (strcmp(name, orig_name) == 0) {
			vorbis_file_set_field_by_name(file, orig_name, value);
		}
		else {
			vorbis_file_set_field_by_name(file, orig_name, "");
			vorbis_file_append_field_by_name(file, name, value);
		}
	}

	g_free(name);
	g_free(value);
}


static gboolean do_modal(int mode, vorbis_file *file, const char *name)
{
	int result;

	/* create the missing widgets and connect the events */
	set_ui(mode, file, name);

	/* show the dialog and wait for it to finish */
	result = gtk_dialog_run(dlg);

	if (result == GTK_RESPONSE_OK)
		update_comments(mode, file, name);

	/* cleanup and return the result */
	gtk_widget_hide(GTK_WIDGET(dlg));
	clear_ui();

	return (result == GTK_RESPONSE_OK);
}


/*** public functions *******************************************************/

gboolean vorbis_editfld_create_comment(vorbis_file *file)
{
	return do_modal(MODE_CREATE, file, NULL);
}


gboolean vorbis_editfld_edit_comment(vorbis_file *file, const char *name)
{
	return do_modal(MODE_EDIT, file, name);
}


void vorbis_editfld_init(GtkBuilder *builder)
{
	dlg = GTK_DIALOG(gtk_builder_get_object(builder, "dlg_edit_field"));
	grid = GTK_GRID(gtk_builder_get_object(builder, "t_edit_field"));
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "ent_edit_field_text"));
	b_ok = GTK_BUTTON(gtk_builder_get_object(builder, "b_edit_field_ok"));
	b_cancel = GTK_BUTTON(gtk_builder_get_object(builder, "b_edit_field_cancel"));

	gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(gtk_builder_get_object(builder, "w_main")));
}

