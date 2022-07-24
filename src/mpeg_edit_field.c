#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <id3.h>

#include "gtk_util.h"
#include "elist.h"
#include "glib_util.h"
#include "mpeg_file.h"
#include "mpeg_edit_field.h"


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
static GtkWidget *widget;  /* dinamically created "field" widget */
static bool changeParent = true;

/* private data */
static GHashTable *frame_id_table = NULL;


/*** private functions ******************************************************/

static void set_ui(int mode, mpeg_file *file, int tag_version, ID3Frame *frame)
{
	const char* title;
	char buffer[256];

	if (mode == MODE_CREATE) {
		GEList *strings;
		int *ids;
		int n_ids;
		int i;

		title = _("New ID3 Field");
		widget = gtk_combo_box_text_new_with_entry();
		//gtk_combo_set_value_in_list(GTK_COMBO_BOX(widget), TRUE, FALSE);
		//gtk_entry_set_editable(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget))), FALSE);
		gtk_editable_set_editable(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(widget))), FALSE);

		frame_id_table = g_hash_table_new(g_str_hash, g_str_equal);

		n_ids = mpeg_file_get_editable_frame_ids(&ids);
		strings = g_elist_new();
		for (i = 0; i < n_ids; i++) {
			const char *name;
			const char *desc;
			char *display_name;
			mpeg_file_get_frame_info(ids[i], &name, &desc, NULL, NULL);
			
			snprintf(buffer, sizeof(buffer), "%s  (%s)", desc, name);
			display_name = strdup(buffer);
			g_hash_table_insert(frame_id_table, display_name, (gpointer)ids[i]);
			g_elist_append(strings, display_name);
		}
		g_elist_sort(strings, (GCompareFunc)strcoll);
		gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(widget));
		g_list_foreach(GLIST(strings), glist_2_combo, GTK_COMBO_BOX_TEXT(widget));
		g_elist_free(strings);
	}
	else {
		const char *name;
		const char *desc;
		const char *text;

		mpeg_file_get_frame_info(ID3Frame_GetID(frame), &name, &desc, NULL, NULL);
		snprintf(buffer, sizeof(buffer), "%s  (%s)", desc, name);

		title = _("Edit ID3 Field");
		widget = gtk_label_new(buffer);
		gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_LEFT);

		mpeg_file_get_frame_text(file, tag_version, frame, &text);
		gtk_entry_set_text(entry, text);
	}
  	if(changeParent) {
	  	gtk_widget_unparent(GTK_WIDGET(b_cancel));
	  	gtk_dialog_add_action_widget(dlg, GTK_WIDGET(b_cancel), GTK_RESPONSE_CANCEL);
	  	gtk_widget_unparent(GTK_WIDGET(b_ok));
	  	gtk_dialog_add_action_widget(dlg, GTK_WIDGET(b_ok), GTK_RESPONSE_OK);

	  	changeParent = false;
	}
	gtk_window_set_title(GTK_WINDOW(dlg), title);
	gtk_grid_attach(grid, widget, 1, 0, 1, 1);

	gtk_widget_show_all(GTK_WIDGET(dlg));
}

static void clear_ui()
{
	if (frame_id_table != NULL) {
		g_hash_table_free(frame_id_table, TRUE, FALSE);
		frame_id_table = NULL;
	}

	gtk_entry_set_text(entry, "");
	gtk_widget_destroy(widget);
}


static void create_frame(mpeg_file *file, int tag_version)
{
	int frame_id;
	ID3Frame *new_frame;
	ID3Tag *tag;

	if (tag_version == ID3TT_ID3V1)
		tag = file->v1_tag;
	else
		tag = file->v2_tag;

	frame_id = (int)g_hash_table_lookup(frame_id_table, 
					    gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget)))));
	new_frame = ID3Frame_NewID(frame_id);
	ID3Tag_AttachFrame(tag, new_frame);

	mpeg_file_set_frame_text(file, tag_version, new_frame, gtk_entry_get_text(entry));
}


static void update_frame(mpeg_file *file, int tag_version, ID3Frame *frame)
{
	mpeg_file_set_frame_text(file, tag_version, frame, gtk_entry_get_text(entry));
}


static gboolean do_modal(int mode, mpeg_file *file, int tag_version, ID3Frame *frame)
{
	int result;

	/* create the missing widgets */
	set_ui(mode, file, tag_version, frame);

	/* show the dialog and wait for it to finish */
	result = gtk_dialog_run(dlg);

	if (result == GTK_RESPONSE_OK) {
		if (mode == MODE_CREATE)
			create_frame(file, tag_version);
		else
			update_frame(file, tag_version, frame);
	}

	/* cleanup and return the result */
	gtk_widget_hide(GTK_WIDGET(dlg));
	clear_ui();

	return (result == GTK_RESPONSE_OK);
}


/*** public functions *******************************************************/

gboolean mpeg_editfld_create_frame(mpeg_file *file, int tag_version)
{
	return do_modal(MODE_CREATE, file, tag_version, NULL);
}


gboolean mpeg_editfld_edit_frame(mpeg_file *file, int tag_version, ID3Frame *frame)
{
	return do_modal(MODE_EDIT, file, tag_version, frame);
}


void mpeg_editfld_init(GtkBuilder *builder)
{
	dlg = GTK_DIALOG(gtk_builder_get_object(builder, "dlg_edit_field"));
	grid = GTK_GRID(gtk_builder_get_object(builder, "t_edit_field"));
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "ent_edit_field_text"));
	b_ok = GTK_BUTTON(gtk_builder_get_object(builder, "b_edit_field_ok"));
	b_cancel = GTK_BUTTON(gtk_builder_get_object(builder, "b_edit_field_cancel"));

	gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(gtk_builder_get_object(builder, "w_main")));
}

