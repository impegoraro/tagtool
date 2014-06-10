#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "elist.h"
#include "math_util.h"
#include "str_util.h"
#include "str_convert.h"
#include "file_util.h"
#include "gtk_util.h"
#include "file_list.h"
#include "progress_dlg.h"
#include "cursor.h"
#include "mru.h"
#include "prefs.h"
#include "audio_file.h"
#include "genre.h"

#include "playlist_tab.h"


enum {
	APPLY_TO_ALL = 0,
	APPLY_TO_SELECTED = 1
};


/* data needed to sort files by their tag fields */
typedef struct {
	gchar *name;
	audio_file *af;
} file_info;

/* widgets */
static GtkButton *b_playlist_go = NULL;
static GtkComboBox *combo_playlist_apply = NULL;
static GtkToggleButton *rb_create_dir = NULL;
static GtkToggleButton *rb_create_toplevel = NULL;
static GtkToggleButton *rb_create_both = NULL;
static GtkToggleButton *rb_name_dir = NULL;
static GtkToggleButton *rb_name_set = NULL;
static GtkToggleButton *rb_sort_name = NULL;
static GtkToggleButton *rb_sort_field = NULL;
static GtkCheckButton *cb_sort_across_dirs = NULL;
static GtkEntry *ent_pl_name = NULL;
static GtkEntry *ent_pl_extension = NULL;
static GtkComboBox *combo_field = NULL;

/* preferences */
static long* create_in;
static gboolean* name_dir;
static char* name;
static char* extension;
static gboolean* sort_by_name;
static long* sort_field;
static gboolean* sort_across_dirs;


/*** private functions ******************************************************/

static gint compare_audio_file(file_info *a, file_info *b)
{
	const char *field_a, *field_b;
	gboolean have_a, have_b;
	int result = 0;

	if (!*sort_across_dirs) {
		/* if sort_across_dirs is FALSE the file's directory takes 
		   precedence over the tag contents */
		if (!fu_compare_file_paths(a->name, b->name)) {
			char *dir_a = g_dirname(a->name);
			char *dir_b = g_dirname(b->name);
			result = strcoll(dir_a, dir_b);
			free(dir_a);
			free(dir_b);
			return result;
		}
	}

	have_a = (audio_file_get_field(a->af, *sort_field, &field_a) == AF_OK);
	if (have_a) field_a = strdup(field_a);
	have_b = (audio_file_get_field(b->af, *sort_field, &field_b) == AF_OK);

	if (have_a ^ have_b) {
		/* files with tags will come before files w/o */
		if (have_a)
			result = -1;
		else
			result = 1;
	}
	else if (have_a & have_b) {
		/* if both have tags, sort by the selected field */
		switch (*sort_field) {
			case AF_TITLE:
			case AF_ARTIST:
			case AF_ALBUM:
			case AF_COMMENT:
			case AF_GENRE:
				result = strcoll(field_a, field_b);
				break;

			case AF_YEAR:
			case AF_TRACK:
				result = compare(atoi(field_a), atoi(field_b));
				break;

			default: /* should never get here */
				g_warning("compare_file_info: unexpected value for *sort_field");
				result = 0;
				break;
		}
	}

	/* if all else is equal, still sort by file name */
	if (result == 0)
		result = strcoll(a->name, b->name);

	if (have_a)
		free((char*)field_a);
	
	return result;
}

static void sort_by_field(GEList *file_list)
{
	file_info *info;
	GEList *file_info_list;
	audio_file *af;
	GList *i, *j;
	int res;

	file_info_list = g_elist_new();

	/* make a list with the file names and tags */
	for (i = g_elist_first(file_list); i; i = g_list_next(i)) {
		res = audio_file_new(&af, i->data, FALSE);
		if (res == AF_OK) {
			info = malloc(sizeof(file_info));
			info->name = i->data;
			info->af = af;
			g_elist_append(file_info_list, info);
		}
		else {
			char *temp_utf8 = str_filename_to_utf8(i->data, _("(UTF8 conversion error)"));
			pd_printf(PD_ICON_WARN, _("Skipping file \"%s\""), temp_utf8);
			free(temp_utf8);
			if (res == AF_ERR_FILE)
				pd_printf(PD_ICON_NONE, _("Couldn't open file for reading"));
			else if (res == AF_ERR_FORMAT)
				pd_printf(PD_ICON_NONE, _("Audio format not recognized"));
			else
				pd_printf(PD_ICON_NONE, _("Unknown error (%d)"), res);
		}
	}

	/* sort the list */
	g_elist_sort(file_info_list, (GCompareFunc)compare_audio_file);

	/* copy the new sort order to the original list */
	while (g_elist_length(file_list) > g_elist_length(file_info_list))
		g_elist_extract(file_list, g_elist_last(file_list));

	i = g_elist_first(file_list);
	j = g_elist_first(file_info_list);
	while (i) {
		i->data = ((file_info *)j->data)->name;

		audio_file_delete(((file_info *)j->data)->af);
		free(j->data);

		i = g_list_next(i);
		j = g_list_next(j);
	}

	g_elist_free(file_info_list);
}


static gchar *playlist_name(gchar *base_dir)
{
	static GString *full_name = NULL;
	gchar *temp_name;
	gchar *temp_ext;

	if (full_name == NULL)
		full_name = g_string_sized_new(100);

	if (gtk_toggle_button_get_active(rb_name_set))
		temp_name = str_filename_from_utf8(name, "utf8_conversion_error");
	else if (*base_dir == 0 || strcmp(base_dir, ".") == 0)
		temp_name = fu_last_path_component(fl_get_working_dir());
	else
		temp_name = fu_last_path_component(base_dir);

	temp_ext = str_filename_from_utf8(extension, "utf8_conversion_error");

	if (*base_dir)
		g_string_sprintf(full_name, "%s/%s.%s", base_dir, temp_name, temp_ext);
	else
		g_string_sprintf(full_name, "%s.%s", temp_name, temp_ext);

	free(temp_name);
	free(temp_ext);

	return full_name->str;
}


static gint write_playlist(gchar *base_dir, gchar *file_name, GEList *file_list)
{
	FILE *f;
	GList *iter;
	gchar *entry;
	gchar *aux;
	gint len = strlen(base_dir);
	gint count = 0;

	f = fopen(file_name, "w");
	if (f == NULL)
		return count;

	/* the list comes sorted by file name... */
	if (gtk_toggle_button_get_active(rb_sort_field)) {
		/* ...sort it by tag field if requested */
		sort_by_field(file_list);
	}

	for (iter = g_elist_first(file_list); iter; iter = g_list_next(iter)) {
		aux = iter->data;
		if (*base_dir && strstr(aux, base_dir) == aux) {
			aux += len;
			while (*aux == '/') aux++;
		}

		entry = strdup(aux);
		str_ascii_replace_char(entry, '/', '\\');
		fprintf(f, "%s\n", entry);
		free(entry);

		count++;
	}
	
	fclose(f);

	return count;
}


static gboolean create_playlist_in_dir(gchar *base_dir, GEList *file_list)
{
	gchar *pl_name;
	gchar *pl_name_display;
	gint ret, save_errno;

	pl_name = playlist_name(base_dir);
	pl_name_display = str_filename_to_utf8(g_basename(pl_name), _("(UTF8 conversion error)"));

	ret = write_playlist(base_dir, pl_name, file_list);
	if (ret == 0) {
		save_errno = errno;
		pd_printf(PD_ICON_FAIL, _("Error creating playlist \"%s\""), pl_name_display);
		pd_printf(PD_ICON_NONE, "%s (%d)", strerror(save_errno), save_errno);
		free(pl_name_display);
		return FALSE;
	}
	else {
		pd_printf(PD_ICON_OK, _("Wrote \"%s\" (%d entries)"), pl_name_display, ret);
		free(pl_name_display);
		return TRUE;
	}
}


static void create_playlists(GEList *file_list)
{
	GEList *aux_list;
	GList *iter;
	GString *path;
	gchar *full_name;
	gchar *last_full_name = "";
	gchar *p;
	gchar *temp_utf8;
	gboolean create_dir;
	gboolean create_toplevel;
	gint count_total, count_written;
	gboolean ret;

	create_dir = gtk_toggle_button_get_active(rb_create_dir) ||
		     gtk_toggle_button_get_active(rb_create_both);
	create_toplevel = gtk_toggle_button_get_active(rb_create_toplevel) ||
			  gtk_toggle_button_get_active(rb_create_both);

	pd_start(_("Writing Playlists"));
	pd_printf(PD_ICON_INFO, _("Starting in directory \"%s\""), fl_get_working_dir_utf8());

	count_total = 0;
	count_written = 0;

	if (create_toplevel) {
		/* make a copy of the list because it may need to be sorted, 
		   and we don't want to change the original */
		aux_list = g_elist_copy(file_list);

		count_total++;
		ret = create_playlist_in_dir("", aux_list);
		if (ret)
			count_written++;

		g_elist_free(aux_list);
	}

	if (create_dir) {
		aux_list = g_elist_new();
		path = g_string_sized_new(200);
		g_string_assign(path, "");

		iter = g_elist_first(file_list);
		while (iter) {
			/* flush pending gtk operations so the UI doesn't freeze */
			pd_scroll_to_bottom();
			while (gtk_events_pending()) gtk_main_iteration();
			if (pd_stop_requested()) {
				pd_printf(PD_ICON_WARN, _("Operation stopped at user's request"));
				break;
			}

			full_name = (gchar*)iter->data;
			if (!fu_compare_file_paths(last_full_name, full_name)) {
				if (!create_toplevel || *(path->str)) {
					count_total++;
					ret = create_playlist_in_dir(path->str, aux_list);
					if (ret)
						count_written++;
				}

				p = strrchr(full_name, '/');
				g_string_sprintf(path, "%.*s", (gint)(p-full_name), full_name);
				g_elist_clear(aux_list);

				temp_utf8 = str_filename_to_utf8(path->str, _("(UTF8 conversion error)"));
				pd_printf(PD_ICON_INFO, _("Entering directory \"%s\""), temp_utf8);
				free(temp_utf8);
			}
			g_elist_append(aux_list, full_name);
			last_full_name = full_name;

			iter = g_list_next(iter);
		}
		if (g_elist_length(aux_list) > 0) {
			/* write the last one */
			count_total++;
			ret = create_playlist_in_dir(path->str, aux_list);
			if (ret)
				count_written++;
		}

		g_elist_free(aux_list);
		g_string_free(path, TRUE);
	}

	pd_printf(PD_ICON_INFO, _("Done (wrote %d of %d playlists)"), count_written, count_total);
	pd_end();
}


/* sets the interface state according to the preferences */
static void from_prefs()
{
	switch (*create_in) {
		case 0: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_create_dir), TRUE);
			break;
		case 1: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_create_toplevel), TRUE);
			break;
		case 2: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_create_both), TRUE);
			break;
	}

	if (*name_dir) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_name_dir), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_pl_name), FALSE);
	}
	else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_name_set), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_pl_name), TRUE);
	}
	gtk_entry_set_text(ent_pl_name, name);
	gtk_entry_set_text(ent_pl_extension, extension);

	if (*sort_by_name) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_sort_name), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(combo_field), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(cb_sort_across_dirs), FALSE);
	}
	else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_sort_field), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(combo_field), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(cb_sort_across_dirs), TRUE);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_sort_across_dirs), *sort_across_dirs);
	gtk_combo_box_set_active(combo_field, *sort_field);
}

/* sets the preferences according to the curent interface state */
static void to_prefs()
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_create_dir)))
		*create_in = 0;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_create_toplevel)))
		*create_in = 1;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_create_both)))
		*create_in = 2;

	*name_dir = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_name_dir));
	name = pref_set("pt:name", PREF_STRING, (void*)gtk_entry_get_text(ent_pl_name));
	extension = pref_set("pt:extension", PREF_STRING, (void*)gtk_entry_get_text(ent_pl_extension));

	*sort_by_name = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_sort_name));
	*sort_across_dirs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_sort_across_dirs));
	*sort_field = gtk_combo_box_get_active(combo_field);
}


/* orchestrates the call to create_playlists() */
static void start_operation()
{
	GEList *file_list;

	if (gtk_combo_box_get_active(combo_playlist_apply) == APPLY_TO_ALL)
		file_list = fl_get_all_files();
	else
		file_list = fl_get_selected_files();

	if (g_elist_length(file_list) == 0) {
		pd_start(_("Writing Playlists"));
		pd_printf(PD_ICON_FAIL, _("No files selected"));
		pd_end();

		g_elist_free(file_list);
		return;
	}

	to_prefs();
	
	cursor_set_wait();
	gtk_widget_set_sensitive(GTK_WIDGET(b_playlist_go), FALSE);
	create_playlists(file_list);
	gtk_widget_set_sensitive(GTK_WIDGET(b_playlist_go), TRUE);
	cursor_set_normal();

	g_elist_free(file_list);
}


/*** UI callbacks ***********************************************************/

void cb_playlist_go(GtkButton *button, gpointer user_data)
{
	start_operation();
}

void cb_name_set(GtkToggleButton *button, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(ent_pl_name),
				 gtk_toggle_button_get_active(button));
}

void cb_sort_field(GtkToggleButton *button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button); 

	gtk_widget_set_sensitive(GTK_WIDGET(combo_field), active);
	gtk_widget_set_sensitive(GTK_WIDGET(cb_sort_across_dirs), active);
}

static void cb_file_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	if (fl_count_selected() > 0)
		gtk_combo_box_set_active(combo_playlist_apply, APPLY_TO_SELECTED);
	else
		gtk_combo_box_set_active(combo_playlist_apply, APPLY_TO_ALL);
}


/*** public functions *******************************************************/

void pt_init(GladeXML *xml)
{
	GtkStyle *style;
	GtkWidget *w;

	/*
	 * get the widgets from glade
	 */

	b_playlist_go = GTK_BUTTON(glade_xml_get_widget(xml, "b_playlist_go"));
	combo_playlist_apply = GTK_COMBO_BOX(glade_xml_get_widget(xml, "combo_playlist_apply"));

	rb_create_dir = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_create_dir"));
	rb_create_toplevel = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_create_toplevel"));
	rb_create_both = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_create_both"));

	rb_name_dir = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_name_dir"));
	rb_name_set = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_name_set"));
	ent_pl_name = GTK_ENTRY(glade_xml_get_widget(xml, "ent_pl_name"));
	ent_pl_extension = GTK_ENTRY(glade_xml_get_widget(xml, "ent_pl_extension"));

	rb_sort_name = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_sort_name"));
	rb_sort_field = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_sort_field"));
	cb_sort_across_dirs = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_sort_across_dirs"));
	combo_field = GTK_COMBO_BOX(glade_xml_get_widget(xml, "combo_field"));

	/* initialize some widgets' state */
	gtk_combo_box_set_active(combo_playlist_apply, APPLY_TO_ALL);

	/* connect signals */
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_files"))),
			 "changed", G_CALLBACK(cb_file_selection_changed), NULL);


	/*
	 * set the title colors
	 */

	w = glade_xml_get_widget(xml, "lab_playlist_title");
	gtk_widget_ensure_style(w);
	style = gtk_widget_get_style(w);

	gtk_widget_modify_fg(w, GTK_STATE_NORMAL, &style->text[GTK_STATE_SELECTED]);

	w = glade_xml_get_widget(xml, "box_playlist_title");
	gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &style->base[GTK_STATE_SELECTED]);


	/*
	 * get the preference values, or set them to defaults
	 */

	/* create_in */
	create_in = pref_get_ref("pt:create_in");
	if (!create_in) {
		long def = 0;
		create_in = pref_set("pt:create_in", PREF_INT, &def);
	}

	/* name_dir */
	name_dir = pref_get_ref("pt:name_dir");
	if (!name_dir) {
		gboolean def = TRUE;
		name_dir = pref_set("pt:name_dir", PREF_BOOLEAN, &def);
	}

	/* name */
	name = pref_get_ref("pt:name");
	if (!name) {
		char *def = "playlist";
		name = pref_set("pt:name", PREF_STRING, def);
	}

	/* extension */
	extension = pref_get_ref("pt:extension");
	if (!extension) {
		char *def = "m3u";
		extension = pref_set("pt:extension", PREF_STRING, def);
	}

	/* sort_by_name */
	sort_by_name = pref_get_ref("pt:sort_by_name");
	if (!sort_by_name) {
		gboolean def = TRUE;
		sort_by_name = pref_set("pt:sort_by_name", PREF_BOOLEAN, &def);
	}

	/* sort_field */
	sort_field = pref_get_ref("pt:sort_field");
	if (!sort_field) {
		long def = 0;
		sort_field = pref_set("pt:sort_field", PREF_INT, &def);
	}

	/* sort_across_dirs */
	sort_across_dirs = pref_get_ref("pt:sort_across_dirs");
	if (!sort_across_dirs) {
		gboolean def = FALSE;
		sort_across_dirs = pref_set("pt:sort_across_dirs", PREF_BOOLEAN, &def);
	}


	from_prefs();
}

