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

#include "str_util.h"
#include "str_convert.h"
#include "file_util.h"
#include "elist.h"
#include "chrono.h"
#include "gtk_util.h"
#include "mru.h"
#include "prefs.h"
#include "cursor.h"
#include "message_box.h"
#include "main_win.h"
#include "status_bar.h"
#include "rename_dlg.h"
#include "progress_dlg.h"
#include "scan_progress_dlg.h"
#include "edit_tab.h"

#include "file_list.h"


/* directory scans that take longer than this (ms) pop up a progress dialog */
#define PROGRESS_DLG_DELAY  1000


/* widgets */
static GtkWindow *w_main = NULL;

static GtkCombo *combo_wd = NULL;
static GtkEntry *ent_wd = NULL;
static GtkToggleButton *cb_recurse = NULL;
static GtkDialog *dlg_wd_select = NULL;

static GtkTreeView *tv_files = NULL;
static GtkListStore *store_files = NULL;
static GtkLabel *lab_file_count = NULL;
static GtkMenu *menu_file_list = NULL;
static GtkMenuItem *m_ctx_manual_rename = NULL;
static GtkMenuItem *m_ctx_delete = NULL;
static GtkMenuItem *m_ctx_unselect_all = NULL;

/* icons */
static GdkPixbuf *pix_file;
static GdkPixbuf *pix_folder;

/* private data */
static GString *working_dir = NULL;
static GString *working_dir_utf8 = NULL;
static GEList *file_list = NULL;
static gchar *editing_file = NULL;
static gboolean progress_dlg_visible;
static chrono_t progress_dlg_time;

/* preferences */
static gboolean *recurse;
static MRUList *dir_mru;


/*** private functions ******************************************************/

static void setup_tree_view()
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* model */
	store_files = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING, 
					 GDK_TYPE_COLOR, G_TYPE_BOOLEAN, G_TYPE_POINTER);

	/* columns and renderers */
	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(tv_files, col);

	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(renderer, "ypad", 0, NULL);
	g_object_set(renderer, "xpad", 4, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", 0);
	gtk_tree_view_column_add_attribute(col, renderer, "cell-background-gdk", 2);
	gtk_tree_view_column_add_attribute(col, renderer, "cell-background-set", 3);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "ypad", 0, NULL);
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
	gtk_tree_view_column_add_attribute(col, renderer, "cell-background-gdk", 2);
	gtk_tree_view_column_add_attribute(col, renderer, "cell-background-set", 3);

	/* selection mode */
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(tv_files),
				    GTK_SELECTION_MULTIPLE);
}


static void update_file_count()
{
	int total;
	int selected;
	GString *str;

	total = file_list->length;
	selected = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(tv_files));

	str = g_string_sized_new(60);

	if (total == 0)
		g_string_assign(str, _("No files found"));
	else if (total == 1)
		g_string_assign(str, _("1 file found"));
	else
		g_string_printf(str, _("%i files found"), total);

	if (selected == 1)
		g_string_append(str, _(" (1 selected)"));
	else if (selected > 1)
		g_string_append_printf(str, _(" (%i selected)"), selected);

	gtk_label_set_text(lab_file_count, str->str);

	g_string_free(str, TRUE);
}


static void update_tree_view(const GEList *file_list)
{
	gchar *last_file = "";
	gchar *aux;
	GtkStyle *style;
	GtkTreeIter tree_iter;
	GList *i;

	style = gtk_widget_get_style(GTK_WIDGET(tv_files));
	if (style == NULL)
		g_error("Couldn't get style for widget tv_files");

	gtk_tree_view_set_model(tv_files, NULL);

	gtk_list_store_clear(store_files);

	i = g_elist_first(file_list);
	while (i) {
		if (!fu_compare_file_paths(last_file, i->data)) {
			char *dirname = g_dirname(i->data);
			aux = str_filename_to_utf8(dirname,
						   _("(dir name could not be converted to UTF8)"));
			gtk_list_store_append(store_files, &tree_iter);
			gtk_list_store_set(store_files, &tree_iter, 
					   0, pix_folder,
					   1, aux,
					   2, &style->bg[GTK_STATE_NORMAL],
					   3, TRUE,
					   4, NULL,
					   -1);
			g_free(dirname);
			g_free(aux);
		}
		aux = str_filename_to_utf8(fu_last_n_path_components(i->data, 1),
					   _("(file name could not be converted to UTF8)"));
		gtk_list_store_append(store_files, &tree_iter);
		gtk_list_store_set(store_files, &tree_iter, 
				   0, pix_file,
				   1, aux,
				   2, NULL,
				   3, FALSE,
				   4, i->data,
				   -1);
		g_free(aux);

		last_file = i->data;
		i = g_list_next(i);
	}

	gtk_tree_view_set_model(tv_files, GTK_TREE_MODEL(store_files));

	update_file_count();
}


static void clear_tree_view()
{
	gtk_list_store_clear(store_files);

	update_file_count();
}


static void popup_tree_view_menu()
{
	int sel_count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(tv_files));

	gtk_widget_set_sensitive(GTK_WIDGET(m_ctx_manual_rename), sel_count == 1);
	gtk_widget_set_sensitive(GTK_WIDGET(m_ctx_delete),        sel_count > 0);
	gtk_widget_set_sensitive(GTK_WIDGET(m_ctx_unselect_all),  sel_count > 0);
	
	gtk_menu_popup(menu_file_list, NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
}


static void delete_files(const GEList *file_list)
{
	GList *iter;
	char *temp_utf8;
	int count_deleted = 0;

	pd_start(_("Deleting Files"));

	for (iter = g_elist_first(file_list); iter; iter = g_list_next(iter)) {
		/* flush pending gtk operations so the UI doesn't freeze */
		pd_scroll_to_bottom();
		while (gtk_events_pending()) gtk_main_iteration();
		if (pd_stop_requested()) {
			pd_printf(PD_ICON_WARN, _("Operation stopped at user's request"));
			break;
		}

		temp_utf8 = str_filename_to_utf8(iter->data, _("(UTF8 conversion error)"));

		if (unlink(iter->data) == 0) {
			count_deleted++;
			pd_printf(PD_ICON_OK, _("Deleted file \"%s\""), temp_utf8);
		}
		else {
			int save_errno = errno;
			pd_printf(PD_ICON_FAIL, _("Error deleting file \"%s\""), temp_utf8);
			pd_printf(PD_ICON_NONE, "%s (%d)", strerror(save_errno), save_errno);
		}

		free(temp_utf8);
	}

	pd_printf(PD_ICON_INFO, _("Done (deleted %d of %d files)"), count_deleted, file_list->length);
	pd_end();

	if (count_deleted > 0)
		fl_refresh(TRUE);
}


static void rename_file(const char *old_path, const char *new_name)
{
	char *new_path;
	char *dir;
	int res;

	dir = g_dirname(old_path);
	new_path = fu_join_path(dir, new_name);
	free(dir);

	if (fu_exists(new_path)) {
		int button;
		button = message_box(w_main, _("File Exists"), 
				     _("A file with this name already exists.\nDo you want to overwrite it?"), 
				     0, GTK_STOCK_CANCEL, GTK_STOCK_YES, NULL);
		if (button == 0) {
			free(new_path);
			return;
		}
	}

	res = rename(old_path, new_path);
	if (res != 0) {
		int save_errno = errno;
		GString *msg = g_string_sized_new(256);

		g_string_printf(msg, _("Error renaming file:\n%s (%d)"), strerror(save_errno), save_errno);
		message_box(w_main, _("Error Renaming File"), msg->str, 0, GTK_STOCK_OK, NULL);

		g_string_free(msg, TRUE);
	}
	else {
		fl_refresh(TRUE);
	}

	free(new_path);
}


static gboolean expand_dir(GString *dir)
{
	gboolean changed = FALSE;

	if (strlen(dir->str) == 0) {
		g_string_assign(dir, g_get_home_dir());
		changed = TRUE;
	}
	if (dir->str[strlen(dir->str)-1] != '/') {
		g_string_append_c(dir, '/');
		changed = TRUE;
	}

	return changed;
}


static gboolean check_working_dir()
{
	gint res;
	struct stat stat_data;

	res = stat(working_dir->str, &stat_data);
	if (res < 0) {
		sb_printf(_("Error: Can't open directory \"%s\"."), working_dir_utf8->str);
		return FALSE;
	}
	if (!S_ISDIR(stat_data.st_mode)) {
		sb_printf(_("Error: \"%s\" is not a directory."), working_dir_utf8->str);
		return FALSE;
	}
	if (!fu_check_permission(&stat_data, "rx")) {
		sb_printf(_("Error: Permission denied for \"%s\"."), working_dir_utf8->str);
		return FALSE;
	}
	return TRUE;
}


static void scan_progress_start()
{
	progress_dlg_visible = FALSE;
	chrono_reset(&progress_dlg_time);
}

static gboolean scan_progress_callback(int dirs, int files)
{
	if (!progress_dlg_visible) {
		if (chrono_get_msec(&progress_dlg_time) > PROGRESS_DLG_DELAY) {
			progress_dlg_visible = TRUE;
			spd_display();
		}
		else {
			return FALSE;
		}
	}

	spd_update(dirs, files);

	/* flush pending gtk operations so the UI doesn't freeze */
	while (gtk_events_pending()) gtk_main_iteration();

	return spd_stop_requested();
}

static void scan_progress_stop()
{
	if (progress_dlg_visible)
		spd_hide();
}


void load_file_list()
{
	static const char *patterns[] = {
#ifdef ENABLE_MP3
		"*.[mM][pP][aA23]",
#endif
#ifdef ENABLE_VORBIS
		"*.[oO][gG][gG]",
#endif
		NULL
	};

	gint count;

	/* clear the form in the edit tab */ 
	et_unload_file();

	sb_printf(_("Scanning..."));
	cursor_set_wait();

	/* flush pending operations before we start */
	while (gtk_events_pending()) gtk_main_iteration();

	/* rebuild the file list */
	if (file_list) {
		g_elist_free_data(file_list);
		file_list = NULL;
	}
	scan_progress_start();
	file_list = fu_get_file_list(working_dir->str, patterns, 
				     scan_progress_callback, 
				     gtk_toggle_button_get_active(cb_recurse),
				     TRUE);
	scan_progress_stop();

	/* update the interface */
	update_tree_view(file_list);
	count = g_elist_length(file_list);
	switch (count) {
	    case 0:
		sb_printf(_("No files found."));
		break;
	    case 1:
		sb_printf(_("1 file found."));
		break;
	    default:
		sb_printf(_("%d files found."), count);
		break;
	}

	cursor_set_normal();
}


void clear_file_list()
{
	if (file_list)
		g_elist_free_data(file_list);
	file_list = g_elist_new();
	clear_tree_view();
}


/*** UI callbacks ***********************************************************/

/* menu callbacks */
void cb_ctx_manual_rename(GtkWidget *widget, GdkEvent *event)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *old_path;
	char *new_name;

	gtk_tree_view_get_first_selected(tv_files, &model, &iter);
	gtk_tree_model_get(model, &iter, 4, &old_path, -1);
	new_name = rename_prompt_new_name(g_basename(old_path));
	if (new_name == NULL)
		return;

	rename_file(old_path, new_name);

	free(new_name);
}

void cb_ctx_delete(GtkWidget *widget, GdkEvent *event)
{
	int count;
	int button;
	GEList *files;

	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(tv_files));
	if (count == 0)
		return;

	button = message_box(w_main, _("Delete selected files"), 
			     _("This will delete the selected files from disk. Proceed?"), 
			     0, GTK_STOCK_CANCEL, GTK_STOCK_YES, NULL);
	if (button == 0)
		return;

	files = fl_get_selected_files();
	delete_files(files);
	g_elist_free(files);
}

void cb_ctx_unselect_all(GtkWidget *widget, GdkEvent *event)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tv_files);
	gtk_tree_selection_unselect_all(selection);
}


/* file list UI callbacks */
static gboolean cb_file_selection_changing(GtkTreeSelection *selection, GtkTreeModel *model, 
					   GtkTreePath *path, gboolean path_currently_selected, 
					   gpointer data)
{
	GtkTreeIter iter;
	gpointer p;

	/* always allow deselection */
	if (path_currently_selected)
		return TRUE;

	/* only allow selection of files (not folders) */
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gtk_tree_model_get(model, &iter, 4, &p, -1);
		if (p == NULL)
			return FALSE;
	}

	return TRUE;
}

static void cb_file_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *new_name;

	update_file_count();

	if (gtk_tree_selection_count_selected_rows(selection) != 1) {
		if (editing_file != NULL) {
			et_unload_file();
			editing_file = NULL;
		}
		return;
	}

	if (gtk_tree_view_get_first_selected(tv_files, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 4, &new_name, -1);
		if (new_name != NULL && (editing_file == NULL || strcmp(new_name, editing_file) != 0)) {
			et_unload_file();
			editing_file = new_name;
			et_load_file(editing_file);
		}
	}
}

gboolean cb_files_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	/* catch right-clicks to popup the context menu */

	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		GtkTreePath *path;
		GtkTreeSelection *selection = gtk_tree_view_get_selection(tv_files);

		gtk_tree_view_get_path_at_pos(tv_files, event->x, event->y, &path, NULL, NULL, NULL);
		if (!gtk_tree_selection_path_is_selected(selection, path)) {
			gtk_tree_selection_unselect_all(selection);
			gtk_tree_selection_select_path(selection, path);
		}
		gtk_tree_path_free(path);

		popup_tree_view_menu();
		return TRUE;
	}

	return FALSE;
}

gboolean cb_files_popup_menu(GtkWidget *widget, gpointer data)
{
	popup_tree_view_menu();
	return TRUE;
}

/* dir selection UI callbacks */
void cb_select_dir(GtkButton *button, gpointer data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg_wd_select), working_dir->str);
	gtk_widget_show(GTK_WIDGET(dlg_wd_select));

	if (gtk_dialog_run(dlg_wd_select) == GTK_RESPONSE_ACCEPT) {
		char *dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg_wd_select));
		fl_set_working_dir(dir);
		g_free(dir);
	}

	gtk_widget_hide(GTK_WIDGET(dlg_wd_select));
}

void cb_wd_changed(GObject *obj, gpointer user_data)
{
	GList *iter;
	const char *new_text = gtk_entry_get_text(ent_wd);

	/* 
	   Problem: GtkCombo doesn't have a signal to indicate the user has
	   selected something, and the underlying GtkList's "selection-changed" 
	   signal is useless as usual (it fires way too many times for no 
	   apparent reason)
	   Solution: Compare the entry text with our MRU list. If there is a 
	   match, that's /probably/ because the user selected something from 
	   the dropdown.
	*/
	for (iter = GLIST(dir_mru->list); iter; iter = g_list_next(iter)) {
		if (strcmp(iter->data, new_text) == 0) {
			fl_set_working_dir_utf8(new_text);
			break;
		}
	}
}

gboolean cb_wd_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_Return) {
		fl_refresh(FALSE);
		return TRUE;
	}

	return FALSE;
}

void cb_toggle_recurse(GtkToggleButton *widget, gpointer data)
{
	*recurse = gtk_toggle_button_get_active(cb_recurse);
	load_file_list();
}


/*** public functions *******************************************************/

void fl_init(GladeXML *xml)
{
	GEList *dir_list;

	/*
	 * get the widgets from glade
	 */
	w_main = GTK_WINDOW(glade_xml_get_widget(xml, "w_main"));
	combo_wd = GTK_COMBO(glade_xml_get_widget(xml, "combo_wd"));
	ent_wd = GTK_ENTRY(glade_xml_get_widget(xml, "ent_wd"));
	cb_recurse = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "cb_recurse"));
	tv_files = GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_files"));
	lab_file_count = GTK_LABEL(glade_xml_get_widget(xml, "lab_file_count"));
	menu_file_list = GTK_MENU(glade_xml_get_widget(xml, "menu_file_list"));
	m_ctx_manual_rename = GTK_MENU_ITEM(glade_xml_get_widget(xml, "m_ctx_manual_rename"));
	m_ctx_delete = GTK_MENU_ITEM(glade_xml_get_widget(xml, "m_ctx_delete"));
	m_ctx_unselect_all = GTK_MENU_ITEM(glade_xml_get_widget(xml, "m_ctx_unselect_all"));

	/* 
	 * create the file chooser
	 */
	dlg_wd_select = GTK_DIALOG(gtk_file_chooser_dialog_new(
					_("Select Directory"),
					w_main,
					GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL));

	/*
	 * load file list icons
	 */
	pix_file = gdk_pixbuf_new_from_file(DATADIR"/file.png", NULL);
	pix_folder = gdk_pixbuf_new_from_file(DATADIR"/folder.png", NULL);

	/*
	 * setup the file list treeview
	 */
	setup_tree_view();
	g_signal_connect(gtk_tree_view_get_selection(tv_files), "changed", 
			 G_CALLBACK(cb_file_selection_changed), NULL);
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(tv_files),
					       cb_file_selection_changing, NULL, NULL);

	/*
	 * get the preference values, or set them to defaults
	 */

	/* recurse */
	recurse = pref_get_ref("ds:recurse");
	if (!recurse) {
		gboolean temp = FALSE;
		recurse = pref_set("ds:recurse", PREF_BOOLEAN, &temp);
	}

	gtk_toggle_button_set_active(cb_recurse, *recurse);

	/* dir_mru */
	dir_list = pref_get_ref("ds:dir_mru");
	if (!dir_list) {
		GEList *temp_list = g_elist_new();
		g_elist_append(temp_list, (void*)g_get_home_dir());
		dir_list = pref_set("ds:dir_mru", PREF_STRING | PREF_LIST, temp_list);
		g_elist_free_data(temp_list);
	}
	dir_mru = mru_new_from_list(10, dir_list);
}


void fl_set_initial_dir(const gchar *dir)
{
	working_dir = g_string_sized_new(200);
	working_dir_utf8 = g_string_sized_new(200);

	if (dir == NULL)
		fl_set_working_dir_utf8(g_elist_first(dir_mru->list)->data);
	else {
		if (g_path_is_absolute(dir))
			fl_set_working_dir(dir);
		else {
			/* make the path absolute */
			char *cur = g_get_current_dir();
			char *abs = fu_join_path(cur, dir);
			fl_set_working_dir(abs);
			free(cur);
			free(abs);
		}
	}
}


void fl_set_working_dir(const gchar *dir)
{
	gchar *aux;

	if (dir == NULL || strcmp(working_dir->str, dir) == 0)
		return;

	/* set the new working_dir */
	g_string_assign(working_dir, dir);
	expand_dir(working_dir);

	aux = str_filename_to_utf8(working_dir->str, NULL);
	if (aux == NULL) {
		/* if we can't convert to UTF-8, leave it as is
		   (let Gtk do the complaining...) */
		g_string_assign(working_dir_utf8, dir);
	}
	else {
		g_string_assign(working_dir_utf8, aux);
		free(aux);
	}
	
	gtk_entry_set_text(ent_wd, working_dir_utf8->str);
	
	/* update the directory mru list */
	mru_add(dir_mru, working_dir_utf8->str);
	gtk_combo_set_popdown_strings(combo_wd, GLIST(dir_mru->list));


	if (check_working_dir())
		load_file_list();
	else
		clear_file_list();
}


void fl_set_working_dir_utf8(const gchar *dir)
{
	gchar *aux;

	if (dir == NULL || strcmp(working_dir_utf8->str, dir) == 0)
		return;

	/* set the new working_dir */
	g_string_assign(working_dir_utf8, dir);

	aux = str_filename_from_utf8(working_dir_utf8->str, NULL);
	if (aux == NULL) {
		/* if we can't convert from UTF-8, leave it as is
		   (loading from the filesystem will just fail) */
		g_string_assign(working_dir, dir);
	}
	else {
		g_string_assign(working_dir, aux);
		free(aux);
	}
	
	if (expand_dir(working_dir)) {
		// need to reflect back to utf
		aux = str_filename_to_utf8(working_dir->str, NULL);
		if (aux != NULL)
			g_string_assign(working_dir_utf8, aux);
		free(aux);
	}
	
	gtk_entry_set_text(ent_wd, working_dir_utf8->str);
	
	/* update the directory mru list */
	mru_add(dir_mru, working_dir_utf8->str);
	gtk_combo_set_popdown_strings(combo_wd, GLIST(dir_mru->list));


	if (check_working_dir())
		load_file_list();
	else
		clear_file_list();
}


const gchar *fl_get_working_dir()
{
	return working_dir->str;
}


const gchar *fl_get_working_dir_utf8()
{
	return working_dir_utf8->str;
}


void fl_refresh(gboolean keep_scroll_pos)
{
	const char *ent_wd_text;
	GtkAdjustment *adj;
	double saved_value;

	adj = gtk_tree_view_get_vadjustment(tv_files);
	saved_value = gtk_adjustment_get_value(adj);

	ent_wd_text = gtk_entry_get_text(ent_wd);
	if (strcmp(working_dir_utf8->str, ent_wd_text) != 0)
		fl_set_working_dir_utf8(ent_wd_text);
	else
		load_file_list();

	if (keep_scroll_pos) {
		if (saved_value <= adj->upper)
			gtk_adjustment_set_value(adj, saved_value);
	}
}


void fl_select_next_file()
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(tv_files);

	if (gtk_tree_selection_count_selected_rows(selection) != 1)
		return;

	if (gtk_tree_view_get_first_selected(tv_files, &model, &iter)) {
		gtk_tree_selection_unselect_iter(selection, &iter);
		if (gtk_tree_model_iter_next(model, &iter))
			gtk_tree_selection_select_iter(selection, &iter);
	}
}


int fl_count_selected()
{
	return gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(tv_files));
}


const gchar *fl_get_selected_file()
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *file;

	gtk_tree_view_get_first_selected(tv_files, &model, &iter);
	gtk_tree_model_get(model, &iter, 4, &file, -1);

	return file;
}


GEList *fl_get_selected_files()
{
	GEList *result = g_elist_new();

	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GList *selected_rows;
	GList *node;

	sel = gtk_tree_view_get_selection(tv_files);
	selected_rows = gtk_tree_selection_get_selected_rows(sel, &model);

	for (node = selected_rows; node; node = node->next) {
		GtkTreeIter iter;
		char *path;

		gtk_tree_model_get_iter(model, &iter, node->data);
		gtk_tree_model_get(model, &iter, 4, &path, -1);

		g_elist_append(result, path);
	}

	g_list_foreach(selected_rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selected_rows);

	return result;
}


GEList *fl_get_all_files()
{
	return g_elist_copy(file_list);
}

