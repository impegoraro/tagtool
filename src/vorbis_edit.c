#include <config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>

#include "elist.h"
#include "gtk_util.h"
#include "mru.h"
#include "prefs.h"
#include "file_list.h"
#include "message_box.h"
#include "cursor.h"
#include "str_util.h"
#include "vorbis_file.h"
#include "vorbis_edit_field.h"

#include "vorbis_edit.h"


/* widgets */
static GtkWindow *w_main = NULL;
static GtkNotebook *nb_edit = NULL;
static GtkNotebook *nb_vorbis = NULL;
static GtkMenuItem *m_vorbis = NULL;
static GtkCheckMenuItem *m_vor_view_simple = NULL;
static GtkCheckMenuItem *m_vor_view_advanced = NULL;

static GtkEntry *ent_title = NULL;
static GtkEntry *ent_artist = NULL;
static GtkEntry *ent_album = NULL;
static GtkEntry *ent_year = NULL;
static GtkEntry *ent_comment = NULL;
static GtkEntry *ent_track = NULL;
static GtkCombo *combo_genre = NULL;
static GtkLabel *lab_title = NULL;
static GtkLabel *lab_artist = NULL;
static GtkLabel *lab_album = NULL;
static GtkLabel *lab_year = NULL;
static GtkLabel *lab_comment = NULL;
static GtkLabel *lab_track = NULL;
static GtkLabel *lab_genre = NULL;
static GtkLabel *lab_advanced = NULL;
static GtkButton *b_advanced = NULL;
static GtkButton *b_clear = NULL;
static GtkButton *b_write = NULL;

static GtkTreeView *tv_comments = NULL;
static GtkListStore *store_comments = NULL;
static GtkButton *b_add = NULL;
static GtkButton *b_edit = NULL;
static GtkButton *b_remove = NULL;
static GtkButton *b_simple = NULL;

/* the various notebook tabs */
#define TAB_SIMPLE 0
#define TAB_ADVANCED 1

/* private data */
static int tab_edit_vorbis;
static gboolean changed_flag = TRUE;
static gboolean editable_flag = TRUE;
static gboolean ignore_changed_signals = FALSE;
static int extra_fields = 0;
static vorbis_file *file = NULL;

/* preferences */
static MRUList *genre_mru;
static int *current_tab;


/*** private functions ******************************************************/

static void tree_view_setup()
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* model */
	store_comments = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

	/* columns and renderers */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Field"));
	gtk_tree_view_append_column(tv_comments, col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ypad", 0, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Text"));
	gtk_tree_view_append_column(tv_comments, col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ypad", 0, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);

	/* selection mode */
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tv_comments),
				    GTK_SELECTION_SINGLE);
}

static void set_changed_flag(gboolean value)
{
	if (changed_flag == value)
		return;

	changed_flag = value;
	gtk_widget_set_sensitive(GTK_WIDGET(b_write), value);
}

static void set_editable_flag(gboolean value)
{
	if (editable_flag == value)
		return;

	editable_flag = value;

	/* simple edit controls */
	gtk_widget_set_sensitive(GTK_WIDGET(ent_title), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_title), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_artist), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_artist), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_album), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_album), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_year), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_year), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_comment), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_comment), value);
	gtk_widget_set_sensitive(GTK_WIDGET(combo_genre), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_genre), value);
	gtk_widget_set_sensitive(GTK_WIDGET(ent_track), value);
	gtk_widget_set_sensitive(GTK_WIDGET(lab_track), value);
	gtk_widget_set_sensitive(GTK_WIDGET(b_clear), value);

	/* advanced edit controls */
	gtk_widget_set_sensitive(GTK_WIDGET(tv_comments), value);
	gtk_widget_set_sensitive(GTK_WIDGET(b_add), value);
	gtk_widget_set_sensitive(GTK_WIDGET(b_edit), value);
	gtk_widget_set_sensitive(GTK_WIDGET(b_remove), value);
}


static gboolean is_simple_field(char *name)
{
	static char *fields[] = {
		"title",
		"artist",
		"album",
		"date",
		"genre",
		"comment",
		"tracknumber",
		NULL
	};
	int i;

	for (i = 0; fields[i] != NULL; i++)
		if (strcmp(name, fields[i]) == 0)
			return TRUE;

	return FALSE;
}


static void count_extra_fields_callback(gchar *name, gchar *text, gpointer data)
{
	if (!is_simple_field(name))
		extra_fields++;
}

static void update_form_simple()
{
	const gchar *str;

	ignore_changed_signals = TRUE;
	if (g_elist_length(genre_mru->list) > 0)
		gtk_combo_set_popdown_strings(combo_genre, GLIST(genre_mru->list));
	vorbis_file_get_field(file, AF_TITLE, &str);   gtk_entry_set_text(ent_title, str);
	vorbis_file_get_field(file, AF_ARTIST, &str);  gtk_entry_set_text(ent_artist, str);
	vorbis_file_get_field(file, AF_ALBUM, &str);   gtk_entry_set_text(ent_album, str);
	vorbis_file_get_field(file, AF_YEAR, &str);    gtk_entry_set_text(ent_year, str);
	vorbis_file_get_field(file, AF_COMMENT, &str); gtk_entry_set_text(ent_comment, str);
	vorbis_file_get_field(file, AF_TRACK, &str);   gtk_entry_set_text(ent_track, str);
	vorbis_file_get_field(file, AF_GENRE, &str);   gtk_entry_set_text(GTK_ENTRY(combo_genre->entry), str);
	ignore_changed_signals = FALSE;

	extra_fields = 0;
	g_hash_table_foreach(file->comments, (GHFunc)count_extra_fields_callback, NULL);

	if (extra_fields > 0) {
		char str[16];
		snprintf(str, 16, _("Advanced (%i)"), extra_fields);
		gtk_label_set_text(lab_advanced, str);
	}
	else {
		gtk_label_set_text(lab_advanced, _("Advanced"));
	}
}

static void append_row_callback(gchar *name, gchar *text, GtkListStore *store)
{
	GtkTreeIter iter;

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, name, 1, text, -1);
}

static void update_form_advanced()
{
	/* fill in the list */
	gtk_tree_view_set_model(tv_comments, NULL);
	gtk_list_store_clear(store_comments);

	g_hash_table_foreach(file->comments, (GHFunc)append_row_callback, store_comments);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store_comments), 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(tv_comments, GTK_TREE_MODEL(store_comments));

	gtk_widget_set_sensitive(GTK_WIDGET(b_edit), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(b_remove), FALSE);
}

static void update_form()
{
	if (!file)
		return;

	if (*current_tab == TAB_SIMPLE)
		update_form_simple();
	else 
		update_form_advanced();
}


static void update_tag()
{
	if (!file || !changed_flag)
		return;

	if (*current_tab == TAB_SIMPLE) {
		vorbis_file_set_field(file, AF_TITLE, gtk_entry_get_text(ent_title));
		vorbis_file_set_field(file, AF_ARTIST, gtk_entry_get_text(ent_artist));
		vorbis_file_set_field(file, AF_ALBUM, gtk_entry_get_text(ent_album));
		vorbis_file_set_field(file, AF_YEAR, gtk_entry_get_text(ent_year));
		vorbis_file_set_field(file, AF_COMMENT, gtk_entry_get_text(ent_comment));
		vorbis_file_set_field(file, AF_TRACK, gtk_entry_get_text(ent_track));
		vorbis_file_set_field(file, AF_GENRE, gtk_entry_get_text(GTK_ENTRY(combo_genre->entry)));
	}
	else {
		/* Nothing to do here, advanced mode alters tag directly */
	}
}


static void write_to_file()
{
	int res;
	const char *genre;

	cursor_set_wait();

	genre = gtk_entry_get_text(GTK_ENTRY(combo_genre->entry));
	if (strcmp(genre, "") != 0)
		mru_add(genre_mru, genre);

	res = vorbis_file_write_changes(file);
	if (res != AF_OK) {
		char msg[512];
		if (errno == EACCES || errno == EPERM) {
			snprintf(msg, sizeof(msg), 
				 _("Error saving file \"%s\":\n%s (%d)\n\n"
				   "Note:\n"
				   "In order to save changes to an Ogg Vorbis file, you must have\n"
				   "write permission for the directory where it is located."), 
				 file->name, strerror(errno), errno);
		}
		else {
			snprintf(msg, sizeof(msg),
				 _("Error saving file \"%s\":\n%s (%d)"), 
				 file->name, strerror(errno), errno);
		}

		message_box(w_main, _("Error Saving File"), msg, 0, GTK_STOCK_OK, NULL);
	}
	else {
		set_changed_flag(FALSE);
	}

	cursor_set_normal();
}



/* UI callbacks */
void cb_vor_clear(GtkButton *button, gpointer user_data)
{
	vorbis_file_remove_tag(file);
	update_form();

	set_changed_flag(TRUE);
}

void cb_vor_write(GtkButton *button, gpointer user_data)
{
	if (!file) {
		g_warning("file is not open!");
		return;
	}

	update_tag();
	write_to_file();
}

void cb_vor_view_simple(GtkWidget *widget, GdkEvent *event)
{
	if (*current_tab != TAB_SIMPLE) {
		update_tag();
		*current_tab = TAB_SIMPLE;
		update_form();
		gtk_notebook_set_page(nb_vorbis, *current_tab);
	}
}

void cb_vor_view_advanced(GtkWidget *widget, GdkEvent *event)
{
	if (*current_tab != TAB_ADVANCED) {
		update_tag();
		*current_tab = TAB_ADVANCED;
		update_form();
		gtk_notebook_set_page(nb_vorbis, *current_tab);
	}
}

void cb_vor_view_button(GtkButton *button, gpointer user_data)
{
	if (*current_tab == TAB_SIMPLE)
		gtk_check_menu_item_set_active(m_vor_view_advanced, TRUE);
	else if (*current_tab == TAB_ADVANCED)
		gtk_check_menu_item_set_active(m_vor_view_simple, TRUE);
}


/* callbacks for the simple editor */
void cb_vor_tag_changed(GObject *obj, gpointer user_data)
{
	if (!ignore_changed_signals)
		set_changed_flag(TRUE);
}

gboolean cb_vor_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
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

/* callbacks for the advanced editor */
void cb_vor_comment_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	if (gtk_tree_selection_count_selected_rows(selection) == 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(b_edit), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(b_remove), FALSE);
	}
	else {
		gtk_widget_set_sensitive(GTK_WIDGET(b_edit), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(b_remove), TRUE);
	}
}

void cb_vor_add(GtkButton *button, gpointer user_data)
{
	if ( vorbis_editfld_create_comment(file) ) {
		set_changed_flag(TRUE);
		update_form();
	}
}

void cb_vor_edit(GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_view_get_first_selected(tv_comments, &model, &iter)) {
		char *name;
		gtk_tree_model_get(model, &iter, 0, &name, -1);

		if ( vorbis_editfld_edit_comment(file, name) ) {
			set_changed_flag(TRUE);
			update_form();
		}
	}
}

void cb_vor_comment_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
	/* same as clicking the edit button */
	cb_vor_edit(NULL, NULL);
}

void cb_vor_remove(GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_view_get_first_selected(tv_comments, &model, &iter)) {
		char *name;
		gtk_tree_model_get(model, &iter, 0, &name, -1);

		/* Setting a comment to empty removes it */
		vorbis_file_set_field_by_name(file, name, "");
		set_changed_flag(TRUE);
		update_form();
	}
}


/*** public functions *******************************************************/

void vorbis_edit_load(vorbis_file *f)
{
	gtk_widget_show(GTK_WIDGET(m_vorbis));

	file = f;
	update_form();

	set_editable_flag(audio_file_is_editable((audio_file *)file));
	set_changed_flag(FALSE);

	gtk_notebook_set_page(nb_edit, tab_edit_vorbis);
	gtk_notebook_set_page(nb_vorbis, *current_tab);
}


void vorbis_edit_unload()
{
	if (changed_flag) {
		int button;

		button = message_box(w_main, _("Save Changes"), 
				     _("Vorbis Tag has been modified. Save changes?"), 1, 
				     _("Discard"), GTK_STOCK_SAVE, NULL);
		if (button == 1) {
			/* save button was pressed */
			update_tag();
			write_to_file();
		}
	}

	file = NULL;

	if (GTK_IS_WIDGET(m_vorbis))
		gtk_widget_hide(GTK_WIDGET(m_vorbis));
}


void vorbis_edit_init(GladeXML *xml)
{
	GEList *temp_list;
	GEList *genre_list;
	int default_tab;

	/*
	 * get the widgets from glade
	 */

	w_main = GTK_WINDOW(glade_xml_get_widget(xml, "w_main"));
	nb_edit = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_edit"));
	nb_vorbis = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_vorbis"));
	m_vorbis = GTK_MENU_ITEM(glade_xml_get_widget(xml, "m_vorbis"));
	m_vor_view_simple = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(xml, "m_vor_view_simple"));
	m_vor_view_advanced = GTK_CHECK_MENU_ITEM(glade_xml_get_widget(xml, "m_vor_view_advanced"));

	ent_title = GTK_ENTRY(glade_xml_get_widget(xml, "ent_vor_title"));
	ent_artist = GTK_ENTRY(glade_xml_get_widget(xml, "ent_vor_artist"));
	ent_album = GTK_ENTRY(glade_xml_get_widget(xml, "ent_vor_album"));
	ent_year = GTK_ENTRY(glade_xml_get_widget(xml, "ent_vor_year"));
	ent_comment = GTK_ENTRY(glade_xml_get_widget(xml, "ent_vor_comment"));
	ent_track = GTK_ENTRY(glade_xml_get_widget(xml, "ent_vor_track"));
	combo_genre = GTK_COMBO(glade_xml_get_widget(xml, "combo_vor_genre"));
	lab_title = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_title"));
	lab_artist = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_artist"));
	lab_album = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_album"));
	lab_year = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_year"));
	lab_comment = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_comment"));
	lab_track = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_track"));
	lab_genre = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_genre"));
	lab_advanced = GTK_LABEL(glade_xml_get_widget(xml, "lab_vor_advanced"));
	b_advanced = GTK_BUTTON(glade_xml_get_widget(xml, "b_vor_advanced"));
	b_clear = GTK_BUTTON(glade_xml_get_widget(xml, "b_vor_clear"));
	b_write = GTK_BUTTON(glade_xml_get_widget(xml, "b_vor_write"));

	tv_comments = GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_vor_comments"));
	b_add = GTK_BUTTON(glade_xml_get_widget(xml, "b_vor_add"));
	b_edit = GTK_BUTTON(glade_xml_get_widget(xml, "b_vor_edit"));
	b_remove = GTK_BUTTON(glade_xml_get_widget(xml, "b_vor_remove"));
	b_simple = GTK_BUTTON(glade_xml_get_widget(xml, "b_vor_simple"));

	/* set up the tree view */
	tree_view_setup();
	g_signal_connect(gtk_tree_view_get_selection(tv_comments), "changed", 
			 G_CALLBACK(cb_vor_comment_selection_changed), NULL);

	/* find out which tab has the Vorbis interface */
	tab_edit_vorbis = gtk_notebook_page_num(nb_edit, glade_xml_get_widget(xml, "cont_vorbis_edit"));


	/*
	 * get the preference values, or set them to defaults
	 */

	/* genre_mru */
	temp_list = g_elist_new();
	genre_list = pref_get_or_set("common_edit:genre_mru", PREF_STRING | PREF_LIST, temp_list);
	g_elist_free_data(temp_list);
	genre_mru = mru_new_from_list(50, genre_list);

	/* current_tab */
	default_tab = TAB_SIMPLE;
	current_tab = pref_get_or_set("vorbis_edit:current_tab", PREF_INT, &default_tab);
	if (*current_tab != TAB_SIMPLE && *current_tab != TAB_ADVANCED)
		*current_tab = default_tab;

	if (*current_tab == TAB_SIMPLE)
		gtk_check_menu_item_set_active(m_vor_view_simple, TRUE);
	else
		gtk_check_menu_item_set_active(m_vor_view_advanced, TRUE);
}

