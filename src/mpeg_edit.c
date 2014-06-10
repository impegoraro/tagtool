#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>

#include "elist.h"
#include "gtk_util.h"
#include "mru.h"
#include "prefs.h"
#include "file_list.h"
#include "message_box.h"
#include "cursor.h"
#include "genre.h"
#include "mpeg_file.h"
#include "mpeg_edit_field.h"

#include "mpeg_edit.h"


/* widgets */
static GtkWindow *w_main = NULL;
static GtkNotebook *nb_edit = NULL;
static GtkNotebook *nb_id3 = NULL;
static GtkNotebook *nb_id3v1 = NULL;
static GtkNotebook *nb_id3v2 = NULL;
static GtkMenuItem *m_id3 = NULL;
static GtkCheckMenuItem *m_id3v2_view_simple = NULL;
static GtkCheckMenuItem *m_id3v2_view_advanced = NULL;
static GtkMenuItem *m_id3_copyv1tov2 = NULL;
static GtkMenuItem *m_id3_copyv2tov1 = NULL;
static GtkButton *b_write_tag = NULL;
static GtkButton *b_remove_tag = NULL;

static GtkButton *b_id3v1_create_tag = NULL;
static GtkEntry *ent_id3v1_title = NULL;
static GtkEntry *ent_id3v1_artist = NULL;
static GtkEntry *ent_id3v1_album = NULL;
static GtkEntry *ent_id3v1_year = NULL;
static GtkEntry *ent_id3v1_comment = NULL;
static GtkSpinButton *spin_id3v1_track = NULL;
static GtkCombo *combo_id3v1_genre = NULL;
static GtkLabel *lab_id3v1_title = NULL;
static GtkLabel *lab_id3v1_artist = NULL;
static GtkLabel *lab_id3v1_album = NULL;
static GtkLabel *lab_id3v1_year = NULL;
static GtkLabel *lab_id3v1_comment = NULL;
static GtkLabel *lab_id3v1_track = NULL;
static GtkLabel *lab_id3v1_genre = NULL;

static GtkButton *b_id3v2_create_tag = NULL;
static GtkEntry *ent_id3v2_title = NULL;
static GtkEntry *ent_id3v2_artist = NULL;
static GtkEntry *ent_id3v2_album = NULL;
static GtkEntry *ent_id3v2_year = NULL;
static GtkTextView *text_id3v2_comment = NULL;
static GtkEntry *ent_id3v2_track = NULL;
static GtkCombo *combo_id3v2_genre = NULL;
static GtkLabel *lab_id3v2_title = NULL;
static GtkLabel *lab_id3v2_artist = NULL;
static GtkLabel *lab_id3v2_album = NULL;
static GtkLabel *lab_id3v2_year = NULL;
static GtkLabel *lab_id3v2_comment = NULL;
static GtkLabel *lab_id3v2_track = NULL;
static GtkLabel *lab_id3v2_genre = NULL;
static GtkLabel *lab_id3v2_advanced = NULL;

static GtkTreeView *tv_id3v2_frames = NULL;
static GtkListStore *store_id3v2_frames = NULL;
static GtkButton *b_id3v2_add = NULL;
static GtkButton *b_id3v2_edit = NULL;
static GtkButton *b_id3v2_remove = NULL;

static GtkImage *img_id3v1_tab = NULL;
static GtkImage *img_id3v2_tab = NULL;
static GdkPixbuf *pix_graydot;
static GdkPixbuf *pix_greendot;


/* the various notebook tabs */
#define TAB_ID3_V1 0
#define TAB_ID3_V2 1
#define TAB_ID3V1_NOTAG 0
#define TAB_ID3V1_TAG 1
#define TAB_ID3V2_NOTAG 0
#define TAB_ID3V2_SIMPLE 1
#define TAB_ID3V2_ADVANCED 2

/* private data */
static int tab_edit_id3;
static gboolean changed_flag = TRUE;
static gboolean editable_flag = TRUE;
static gboolean ignore_changed_signals = FALSE;
static mpeg_file *file = NULL;

/* preferences */
static MRUList *genre_mru;
static int *current_tab;


/*** private functions ******************************************************/

static void tree_view_setup()
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* model */
	store_id3v2_frames = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

	/* columns and renderers */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Field"));
	gtk_tree_view_append_column(tv_id3v2_frames, col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ypad", 0, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Text"));
	gtk_tree_view_append_column(tv_id3v2_frames, col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ypad", 0, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);

	/* selection mode */
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tv_id3v2_frames),
				    GTK_SELECTION_SINGLE);
}

static void set_changed_flag(gboolean value)
{
	if (changed_flag == value)
		return;

	changed_flag = value;
	gtk_widget_set_sensitive(GTK_WIDGET(b_write_tag), value);
}

static void set_editable_flag(gboolean value)
{
	if (editable_flag == value)
		return;

	editable_flag = value;
	gtk_widget_set_sensitive(GTK_WIDGET(b_remove_tag), value);

	/* v1 edit controls */
	gtk_widget_set_sensitive(GTK_WIDGET(b_id3v1_create_tag), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v1_title), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v1_title), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v1_artist), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v1_artist), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v1_album), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v1_album), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v1_year), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v1_year), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v1_comment), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v1_comment), value);
	gtk_widget_set_sensitive(GTK_WIDGET(combo_id3v1_genre), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v1_genre), value);
	gtk_widget_set_sensitive(GTK_WIDGET(spin_id3v1_track), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v1_track), value);
	gtk_widget_set_sensitive(GTK_WIDGET(m_id3_copyv1tov2), value);

	/* v2 simple edit controls */
	gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_create_tag), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v2_title), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v2_title), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v2_artist), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v2_artist), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v2_album), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v2_album), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v2_year), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v2_year), value);
	gtk_widget_set_sensitive(GTK_WIDGET(text_id3v2_comment), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v2_comment), value);
	gtk_widget_set_sensitive(GTK_WIDGET(combo_id3v2_genre), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v2_genre), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_id3v2_track), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_id3v2_track), value);
	gtk_widget_set_sensitive(GTK_WIDGET(m_id3_copyv2tov1), value);

	/* v2 advanced edit controls */
	gtk_widget_set_sensitive(GTK_WIDGET(tv_id3v2_frames), value);
	gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_add), value);
	gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_edit), value);
	gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_remove), value);
}

static void update_form_v1()
{
	const gchar *str;

	if (mpeg_file_has_tag_v(file, ID3TT_ID3V1)) {
		gtk_notebook_set_page(nb_id3v1, TAB_ID3V1_TAG);

		ignore_changed_signals = TRUE;
		mpeg_file_get_field_v(file, ID3TT_ID3V1, AF_TITLE, &str);
		gtk_entry_set_text(ent_id3v1_title, str);
		mpeg_file_get_field_v(file, ID3TT_ID3V1, AF_ARTIST, &str);
		gtk_entry_set_text(ent_id3v1_artist, str);
		mpeg_file_get_field_v(file, ID3TT_ID3V1, AF_ALBUM, &str);
		gtk_entry_set_text(ent_id3v1_album, str);
		mpeg_file_get_field_v(file, ID3TT_ID3V1, AF_YEAR, &str);
		gtk_entry_set_text(ent_id3v1_year, str);
		mpeg_file_get_field_v(file, ID3TT_ID3V1, AF_COMMENT, &str);
		gtk_entry_set_text(ent_id3v1_comment, str);
		mpeg_file_get_field_v(file, ID3TT_ID3V1, AF_TRACK, &str);
		gtk_spin_button_set_value(spin_id3v1_track, atoi(str));
		mpeg_file_get_field_v(file, ID3TT_ID3V1, AF_GENRE, &str);
		gtk_entry_set_text(GTK_ENTRY(combo_id3v1_genre->entry), str);
		ignore_changed_signals = FALSE;
	}
	else {
		gtk_notebook_set_page(nb_id3v1, TAB_ID3V1_NOTAG);
	}

	set_editable_flag(audio_file_is_editable((audio_file *)file));
}

static void update_form_v2()
{
	const gchar *str;

	if (mpeg_file_has_tag_v(file, ID3TT_ID3V2)) {
		gtk_notebook_set_page(nb_id3v2, *current_tab);
		
		if (*current_tab == TAB_ID3V2_SIMPLE) {
			int extra_fields;
			ID3Frame **frames;
			int nframes;
			int i;

			ignore_changed_signals = TRUE;
			if (g_elist_length(genre_mru->list) > 0)
				gtk_combo_set_popdown_strings(combo_id3v2_genre, GLIST(genre_mru->list));
			mpeg_file_get_field_v(file, ID3TT_ID3V2, AF_TITLE, &str);
			gtk_entry_set_text(ent_id3v2_title, str);
			mpeg_file_get_field_v(file, ID3TT_ID3V2, AF_ARTIST, &str);
			gtk_entry_set_text(ent_id3v2_artist, str);
			mpeg_file_get_field_v(file, ID3TT_ID3V2, AF_ALBUM, &str);
			gtk_entry_set_text(ent_id3v2_album, str);
			mpeg_file_get_field_v(file, ID3TT_ID3V2, AF_YEAR, &str);
			gtk_entry_set_text(ent_id3v2_year, str);
			mpeg_file_get_field_v(file, ID3TT_ID3V2, AF_COMMENT, &str);
			gtk_text_buffer_set_text(gtk_text_view_get_buffer(text_id3v2_comment), str, -1);
			mpeg_file_get_field_v(file, ID3TT_ID3V2, AF_TRACK, &str);
			gtk_entry_set_text(ent_id3v2_track, str);
			mpeg_file_get_field_v(file, ID3TT_ID3V2, AF_GENRE, &str);
			gtk_entry_set_text(GTK_ENTRY(combo_id3v2_genre->entry), str);
			ignore_changed_signals = FALSE;

			/* Show in the "Advanced" button label how many fields are not visible */
			extra_fields = 0;
			nframes = mpeg_file_get_frames(file, ID3TT_ID3V2, &frames);
			for (i = 0; i < nframes; i++) {
				gboolean is_simple;
				mpeg_file_get_frame_info(ID3Frame_GetID(frames[i]), NULL, NULL, NULL, &is_simple);
				if (!is_simple)
					extra_fields++;
			}

			if (extra_fields > 0) {
				char str[16];
				snprintf(str, sizeof(str), _("Advanced (%i)"), extra_fields);
				gtk_label_set_text(lab_id3v2_advanced, str);
			}
			else {
				gtk_label_set_text(lab_id3v2_advanced, _("Advanced"));
			}
		}
		else {
			ID3Frame **frames;
			int nframes;
			int i;

			nframes = mpeg_file_get_frames(file, ID3TT_ID3V2, &frames);
			
			/* fill in the list */
			gtk_tree_view_set_model(tv_id3v2_frames, NULL);
			gtk_list_store_clear(store_id3v2_frames);

			for (i = 0; i < nframes; i++) {
				char short_desc[64];
				const char *value;
				const char *desc, *p;
				GtkTreeIter iter;

				mpeg_file_get_frame_info(ID3Frame_GetID(frames[i]), NULL, &desc, NULL, NULL);
				p = strchr(desc, '/');
				snprintf(short_desc, sizeof(short_desc), "%.*s", (p ? p-desc : sizeof(short_desc)), desc);
				
				mpeg_file_get_frame_text(file, ID3TT_ID3V2, frames[i], &value);
				if (value == NULL)
					value = "[non-text field]";

				gtk_list_store_append(store_id3v2_frames, &iter);
				gtk_list_store_set(store_id3v2_frames, &iter, 
						   0, short_desc,
						   1, value,
						   2, frames[i],
						   -1);
			}
			gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store_id3v2_frames), 0, GTK_SORT_ASCENDING);

			gtk_tree_view_set_model(tv_id3v2_frames, GTK_TREE_MODEL(store_id3v2_frames));

			free(frames);

			gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_edit), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_remove), FALSE);
		}
	}
	else {
		gtk_notebook_set_page(nb_id3v2, TAB_ID3V2_NOTAG);
	}

	set_editable_flag(audio_file_is_editable((audio_file *)file));
}

static void update_tag_from_form_v1(int tag)
{
	mpeg_file_set_field_v(file, tag, AF_TITLE, gtk_entry_get_text(ent_id3v1_title));
	mpeg_file_set_field_v(file, tag, AF_ARTIST, gtk_entry_get_text(ent_id3v1_artist));
	mpeg_file_set_field_v(file, tag, AF_ALBUM, gtk_entry_get_text(ent_id3v1_album));
	mpeg_file_set_field_v(file, tag, AF_YEAR, gtk_entry_get_text(ent_id3v1_year));
	mpeg_file_set_field_v(file, tag, AF_COMMENT, gtk_entry_get_text(ent_id3v1_comment));
	mpeg_file_set_field_v(file, tag, AF_TRACK, gtk_entry_get_text(GTK_ENTRY(spin_id3v1_track)));
	mpeg_file_set_field_v(file, tag, AF_GENRE, gtk_entry_get_text(GTK_ENTRY(combo_id3v1_genre->entry)));
}

static void update_tag_from_form_v2(int tag)
{
	if (*current_tab == TAB_ID3V2_SIMPLE) {
		GtkTextIter start, end;
		gchar *temp;

		mpeg_file_set_field_v(file, tag, AF_TITLE, gtk_entry_get_text(ent_id3v2_title));
		mpeg_file_set_field_v(file, tag, AF_ARTIST, gtk_entry_get_text(ent_id3v2_artist));
		mpeg_file_set_field_v(file, tag, AF_ALBUM, gtk_entry_get_text(ent_id3v2_album));
		mpeg_file_set_field_v(file, tag, AF_YEAR, gtk_entry_get_text(ent_id3v2_year));
		mpeg_file_set_field_v(file, tag, AF_TRACK, gtk_entry_get_text(ent_id3v2_track));
		mpeg_file_set_field_v(file, tag, AF_GENRE, gtk_entry_get_text(GTK_ENTRY(combo_id3v2_genre->entry)));

		gtk_text_buffer_get_bounds(gtk_text_view_get_buffer(text_id3v2_comment), &start, &end);
		temp = gtk_text_buffer_get_text(gtk_text_view_get_buffer(text_id3v2_comment), &start, &end, FALSE);
		mpeg_file_set_field_v(file, tag, AF_COMMENT, temp);
		g_free(temp);
	}
	else {
		/* Nothing to do here, advanced mode alters tag directly */
	}
}

static void update_tag()
{
	if (mpeg_file_has_tag_v(file, ID3TT_ID3V1))
		update_tag_from_form_v1(ID3TT_ID3V1);

	if (mpeg_file_has_tag_v(file, ID3TT_ID3V2))
		update_tag_from_form_v2(ID3TT_ID3V2);
}

static void update_tabs(gboolean allow_change)
{
	if (mpeg_file_has_tag_v(file, ID3TT_ID3V1)) {
		gtk_image_set_from_pixbuf(img_id3v1_tab, pix_greendot);
		gtk_widget_set_sensitive(GTK_WIDGET(m_id3_copyv1tov2), TRUE);
	}
	else {
		gtk_image_set_from_pixbuf(img_id3v1_tab, pix_graydot);
		gtk_widget_set_sensitive(GTK_WIDGET(m_id3_copyv1tov2), FALSE);
	}

	if (mpeg_file_has_tag_v(file, ID3TT_ID3V2)) {
		gtk_image_set_from_pixbuf(img_id3v2_tab, pix_greendot);
		gtk_widget_set_sensitive(GTK_WIDGET(m_id3_copyv2tov1), TRUE);
	}
	else {
		gtk_image_set_from_pixbuf(img_id3v2_tab, pix_graydot);
		gtk_widget_set_sensitive(GTK_WIDGET(m_id3_copyv2tov1), FALSE);
	}

	gtk_notebook_set_page(nb_edit, tab_edit_id3);
	if (allow_change) {
		if (mpeg_file_has_tag_v(file, ID3TT_ID3V2))
			gtk_notebook_set_page(nb_id3, TAB_ID3_V2);
		else
			gtk_notebook_set_page(nb_id3, TAB_ID3_V1);
	}
}

static void write_to_file()
{
	int res;

	cursor_set_wait();

	if (mpeg_file_has_tag_v(file, ID3TT_ID3V2)) {
		const char *genre = gtk_entry_get_text(GTK_ENTRY(combo_id3v2_genre->entry));
		if (strcmp(genre, "") != 0)
			mru_add(genre_mru, genre);
	}

	res = mpeg_file_write_changes(file);
	if (res != AF_OK) {
		char msg[512];
		snprintf(msg, sizeof(msg), 
			 _("Error saving file \"%s\":\n%s (%d)"), 
			 file->name, strerror(errno), errno);
		message_box(w_main, _("Error Saving File"), msg, 0, GTK_STOCK_OK, NULL);
	}
	else {
		set_changed_flag(FALSE);
	}

	cursor_set_normal();
}


/*** UI callbacks ***********************************************************/

void cb_id3_write_changes(GtkButton *button, gpointer user_data)
{
	if (!file) {
		g_warning("file is not open!");
		return;
	}

	update_tag();
	write_to_file();
}

void cb_id3_remove_tag(GtkButton *button, gpointer user_data)
{
	if (!file) {
		g_warning("file is not open!");
		return;
	}

	if (gtk_notebook_get_current_page(nb_id3) == TAB_ID3_V1) {
		mpeg_file_remove_tag_v(file, ID3TT_ID3V1);
		update_form_v1();
	}
	else {
		mpeg_file_remove_tag_v(file, ID3TT_ID3V2);
		update_form_v2();
	}

	update_tabs(TRUE);
	set_changed_flag(TRUE);
}

void cb_id3v1_create_tag(GtkButton *button, gpointer user_data)
{
	if (!file) {
		g_warning("file is not open!");
		return;
	}

	mpeg_file_create_tag_v(file, ID3TT_ID3V1);
	set_changed_flag(TRUE);
	update_form_v1();
	update_tabs(FALSE);
}

void cb_id3v2_create_tag(GtkButton *button, gpointer user_data)
{
	if (!file) {
		g_warning("file is not open!");
		return;
	}

	mpeg_file_create_tag_v(file, ID3TT_ID3V2);
	set_changed_flag(TRUE);
	update_form_v2();
	update_tabs(FALSE);
}

/* v1 and v2 simple mode callbacks */
void cb_id3_tag_changed(GObject *obj, gpointer user_data)
{
	if (!ignore_changed_signals)
		set_changed_flag(TRUE);
}

gboolean cb_id3_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	/* <return> saves the tag and advances to the next file */
	if (event->keyval == GDK_Return) {
		if (file && changed_flag) {
			update_tag();
			write_to_file();
		}
		fl_select_next_file();
		return TRUE;
	}

	return FALSE;
}

/* v2 advanced mode callbacks */
void cb_id3v2_frame_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	if (gtk_tree_selection_count_selected_rows(selection) == 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_edit), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_remove), FALSE);
	}
	else {
		GtkTreeModel *model;
		GtkTreeIter iter;
		ID3Frame *frame;
		gboolean editable;

		gtk_tree_view_get_first_selected(tv_id3v2_frames, &model, &iter);
		gtk_tree_model_get(model, &iter, 2, &frame, -1);

		mpeg_file_get_frame_info(ID3Frame_GetID(frame), NULL, NULL, &editable, NULL);
	
		gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_edit), editable);
		gtk_widget_set_sensitive(GTK_WIDGET(b_id3v2_remove), TRUE);
	}
}

void cb_id3v2_add(GtkButton *button, gpointer user_data)
{
	if ( mpeg_editfld_create_frame(file, ID3TT_ID3V2) ) {
		set_changed_flag(TRUE);
		update_form_v2();
	}
}

void cb_id3v2_edit(GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_view_get_first_selected(tv_id3v2_frames, &model, &iter)) {
		ID3Frame *frame;
		gboolean editable;

		gtk_tree_model_get(model, &iter, 2, &frame, -1);

		mpeg_file_get_frame_info(ID3Frame_GetID(frame), NULL, NULL, &editable, NULL);
		if (!editable)
			return;

		if ( mpeg_editfld_edit_frame(file, ID3TT_ID3V2, frame) ) {
			set_changed_flag(TRUE);
			update_form_v2();
		}
	}
}

void cb_id3v2_frame_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
	/* same as clicking the edit button */
	cb_id3v2_edit(NULL, NULL);
}

void cb_id3v2_remove(GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_view_get_first_selected(tv_id3v2_frames, &model, &iter)) {
		ID3Frame *frame;
		gtk_tree_model_get(model, &iter, 2, &frame, -1);

		mpeg_file_remove_frame(file, ID3TT_ID3V2, frame);
		set_changed_flag(TRUE);
		update_form_v2();
	}
}

void cb_id3v2_view_button(GtkButton *button, gpointer user_data)
{
	if (*current_tab == TAB_ID3V2_SIMPLE)
		gtk_check_menu_item_set_active(m_id3v2_view_advanced, TRUE);
	else if (*current_tab == TAB_ID3V2_ADVANCED)
		gtk_check_menu_item_set_active(m_id3v2_view_simple, TRUE);
}

/* menu callbacks */
void cb_id3v2_view_simple(GtkWidget *widget, GdkEvent *event)
{
	if (*current_tab == TAB_ID3V2_ADVANCED) {
		*current_tab = TAB_ID3V2_SIMPLE;
		if (gtk_notebook_get_current_page(nb_id3v2) != TAB_ID3V2_NOTAG) {
			update_form_v2();
			gtk_notebook_set_page(nb_id3v2, TAB_ID3V2_SIMPLE);
		}
	}
}

void cb_id3v2_view_advanced(GtkWidget *widget, GdkEvent *event)
{
	if (*current_tab == TAB_ID3V2_SIMPLE) {
		update_tag_from_form_v2(ID3TT_ID3V2);

		*current_tab = TAB_ID3V2_ADVANCED;
		if (gtk_notebook_get_current_page(nb_id3v2) != TAB_ID3V2_NOTAG) {
			update_form_v2();
			gtk_notebook_set_page(nb_id3v2, TAB_ID3V2_ADVANCED);
		}
	}
}

void cb_id3_copyv1tov2(GtkWidget *widget, GdkEvent *event)
{
	if (!file) {
		g_warning("file is not open!");
		return;
	}

	if (!mpeg_file_has_tag_v(file, ID3TT_ID3V2))
		mpeg_file_create_tag_v(file, ID3TT_ID3V2);

	update_tag_from_form_v1(ID3TT_ID3V2);

	set_changed_flag(TRUE);
	update_form_v2();
	update_tabs(FALSE);
}

void cb_id3_copyv2tov1(GtkWidget *widget, GdkEvent *event)
{
	if (!file) {
		g_warning("file is not open!");
		return;
	}

	if (!mpeg_file_has_tag_v(file, ID3TT_ID3V1))
		mpeg_file_create_tag_v(file, ID3TT_ID3V1);

	update_tag_from_form_v2(ID3TT_ID3V1);

	set_changed_flag(TRUE);
	update_form_v1();
	update_tabs(FALSE);
}


/*** public functions *******************************************************/

void mpeg_edit_load(mpeg_file *f)
{
	gtk_widget_show(GTK_WIDGET(m_id3));

	file = f;
	set_changed_flag(FALSE);
	update_form_v1();
	update_form_v2();
	update_tabs(TRUE);
}


void mpeg_edit_unload()
{
	if (changed_flag) {
		int button;

		button = message_box(w_main, _("Save Changes"), 
				     _("ID3 Tag has been modified. Save changes?"), 1, 
				     _("Discard"), GTK_STOCK_SAVE, NULL);
		if (button == 1) {
			/* save button was pressed */
			update_tag();
			write_to_file();
		}
	}

	file = NULL;

	if (GTK_IS_WIDGET(m_id3))
		gtk_widget_hide(GTK_WIDGET(m_id3));
}


void mpeg_edit_init(GladeXML *xml)
{
	GEList *temp_list;
	GEList *genre_list;
	int default_tab = TAB_ID3V2_SIMPLE;

	/* get the widgets from glade */
	w_main = GTK_WINDOW(glade_xml_get_widget(xml, "w_main"));
	nb_edit = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_edit"));
	nb_id3 = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_id3"));
	nb_id3v1 = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_id3v1"));
	nb_id3v2 = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_id3v2"));
	m_id3 = GTK_MENU_ITEM(glade_xml_get_widget(xml, "m_id3"));
	m_id3v2_view_simple = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(xml, "m_id3v2_view_simple"));
	m_id3v2_view_advanced = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(xml, "m_id3v2_view_advanced"));
	m_id3_copyv1tov2 = GTK_MENU_ITEM(glade_xml_get_widget(xml, "m_id3_copyv1tov2"));
	m_id3_copyv2tov1 = GTK_MENU_ITEM(glade_xml_get_widget(xml, "m_id3_copyv2tov1"));
	b_write_tag = GTK_BUTTON(glade_xml_get_widget(xml, "b_id3_write_tag"));
	b_remove_tag = GTK_BUTTON(glade_xml_get_widget(xml, "b_id3_remove_tag"));

	b_id3v1_create_tag = GTK_BUTTON(glade_xml_get_widget(xml, "b_id3v1_create_tag"));
	ent_id3v1_title = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v1_title"));
	ent_id3v1_artist = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v1_artist"));
	ent_id3v1_album = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v1_album"));
	ent_id3v1_year = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v1_year"));
	ent_id3v1_comment = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v1_comment"));
	spin_id3v1_track = GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "spin_id3v1_track"));
	combo_id3v1_genre = GTK_COMBO(glade_xml_get_widget(xml, "combo_id3v1_genre"));
	lab_id3v1_title = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v1_title"));
	lab_id3v1_artist = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v1_artist"));
	lab_id3v1_album = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v1_album"));
	lab_id3v1_year = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v1_year"));
	lab_id3v1_comment = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v1_comment"));
	lab_id3v1_track = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v1_track"));
	lab_id3v1_genre = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v1_genre"));

	b_id3v2_create_tag = GTK_BUTTON(glade_xml_get_widget(xml, "b_id3v2_create_tag"));
	ent_id3v2_title = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v2_title"));
	ent_id3v2_artist = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v2_artist"));
	ent_id3v2_album = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v2_album"));
	ent_id3v2_year = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v2_year"));
	text_id3v2_comment = GTK_TEXT_VIEW(glade_xml_get_widget(xml, "text_id3v2_comment"));
	ent_id3v2_track = GTK_ENTRY(glade_xml_get_widget(xml, "ent_id3v2_track"));
	combo_id3v2_genre = GTK_COMBO(glade_xml_get_widget(xml, "combo_id3v2_genre"));
	lab_id3v2_title = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_title"));
	lab_id3v2_artist = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_artist"));
	lab_id3v2_album = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_album"));
	lab_id3v2_year = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_year"));
	lab_id3v2_comment = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_comment"));
	lab_id3v2_track = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_track"));
	lab_id3v2_genre = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_genre"));
	lab_id3v2_advanced = GTK_LABEL(glade_xml_get_widget(xml, "lab_id3v2_advanced"));

	tv_id3v2_frames = GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_id3v2_frames"));
	b_id3v2_add = GTK_BUTTON(glade_xml_get_widget(xml, "b_id3v2_add"));
	b_id3v2_edit = GTK_BUTTON(glade_xml_get_widget(xml, "b_id3v2_edit"));
	b_id3v2_remove = GTK_BUTTON(glade_xml_get_widget(xml, "b_id3v2_remove"));

	img_id3v1_tab = GTK_IMAGE(glade_xml_get_widget(xml, "img_id3v1_tab"));
	img_id3v2_tab = GTK_IMAGE(glade_xml_get_widget(xml, "img_id3v2_tab"));

	/* additional widget setup */
	gtk_editable_set_max_chars(GTK_EDITABLE(ent_id3v1_title), 30);
	gtk_editable_set_max_chars(GTK_EDITABLE(ent_id3v1_artist), 30);
	gtk_editable_set_max_chars(GTK_EDITABLE(ent_id3v1_album), 30);
	gtk_editable_set_max_chars(GTK_EDITABLE(ent_id3v1_comment), 29);
	gtk_editable_set_max_chars(GTK_EDITABLE(ent_id3v1_year), 4);

	g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(text_id3v2_comment)), 
			 "changed", G_CALLBACK(cb_id3_tag_changed), NULL);

	/* load the pixbufs */
	pix_graydot = gdk_pixbuf_new_from_file(DATADIR"/graydot.png", NULL);
	pix_greendot = gdk_pixbuf_new_from_file(DATADIR"/greendot.png", NULL);

	/* set up the tree view */
	tree_view_setup();
	g_signal_connect(gtk_tree_view_get_selection(tv_id3v2_frames), "changed", 
			 G_CALLBACK(cb_id3v2_frame_selection_changed), NULL);

	/* find out which tab has the ID3 interface */
	tab_edit_id3 = gtk_notebook_page_num(nb_edit, glade_xml_get_widget(xml, "cont_id3_edit"));

	/* ID3v1 genre list */
	temp_list = genre_create_list(TRUE);
	g_elist_prepend(temp_list, "");
	gtk_combo_set_popdown_strings(combo_id3v1_genre, GLIST(temp_list));
	g_elist_free(temp_list);

	/* ID3v2 genre list, from prefs */
	temp_list = g_elist_new();
	genre_list = pref_get_or_set("common_edit:genre_mru", PREF_STRING | PREF_LIST, temp_list);
	g_elist_free_data(temp_list);
	genre_mru = mru_new_from_list(50, genre_list);

	/* current_tab, from prefs */
	current_tab = pref_get_or_set("id3_edit:current_tab", PREF_INT, &default_tab);
	if (*current_tab != TAB_ID3V2_SIMPLE && *current_tab != TAB_ID3V2_ADVANCED)
		*current_tab = default_tab;

	if (*current_tab == TAB_ID3V2_SIMPLE)
		gtk_check_menu_item_set_active(m_id3v2_view_simple, TRUE);
	else if (*current_tab == TAB_ID3V2_ADVANCED)
		gtk_check_menu_item_set_active(m_id3v2_view_advanced, TRUE);
}

