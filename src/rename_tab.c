#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "elist.h"
#include "str_util.h"
#include "str_convert.h"
#include "file_util.h"
#include "mru.h"
#include "prefs.h"
#include "audio_file.h"
#include "file_list.h"
#include "char_conv_dlg.h"
#include "progress_dlg.h"
#include "cursor.h"
#include "help.h"

#include "rename_tab.h"


enum {
	APPLY_TO_ALL = 0,
	APPLY_TO_SELECTED = 1
};


/* widgets */
static GtkCombo *combo_rename_format = NULL;
static GtkEntry *ent_rename_format = NULL;
static GtkButton *b_rename_edit_format = NULL;
static GtkButton *b_rename_go = NULL;
static GtkComboBox *combo_rename_apply = NULL;

/* preferences */
static MRUList *format_mru;


/*** private functions ******************************************************/

static gboolean append_field(GString *gstr, audio_file *af, int field)
{
	char *temp;
	char *value;
	chconv_rename_options options;

	audio_file_get_field(af, field, (const char **)&temp);
	if (temp == NULL || *temp == 0)
		return FALSE;

	/* apply character conversions */
	options = chconv_get_rename_options();

	if (options.invalid_conv != NULL && options.invalid_conv[0] != 0)
		value = str_replace_char(temp, '/', options.invalid_conv[0]);
	else
		value = str_remove_char(temp, '/');

	if (options.space_conv != NULL && options.space_conv[0] != 0) {
		temp = value;
		value = str_replace_char(value, ' ', options.space_conv[0]);
		free(temp);
	}

	if (options.case_conv != CASE_CONV_NONE) {
		temp = value;
		value = str_convert_case(value, options.case_conv);
		free(temp);
	}

	str_trim(value);

	/* convert to filesystem encoding */
	temp = value;
	value = str_filename_from_utf8(value, "utf8_conversion_error");
	free(temp);

	/* append to name */
	if (field == AF_TRACK) {
		long track;
		char *endptr;
		track = strtol(value, &endptr, 10);
		if (*endptr == 0)
			g_string_sprintfa(gstr, "%02li", track);
		else
			g_string_append(gstr, value);
	}
	else {
		g_string_append(gstr, value);
	}

	free(value);

	return TRUE;
}

static gboolean build_file_name(const gchar *format, audio_file *af, GString *new_name)
{
	gchar *p;
	gint i, span;

	/* fill in the filename from the tag according to the given format */
	i = 0;
	while (TRUE) {
		p = index(&format[i], '<');
		if (p != NULL) {
			span = (gint)(p - &format[i]);
			if (span > 0) {
				g_string_sprintfa(new_name, "%.*s", span, &format[i]);
				i += span;
			} else if (strncmp(&format[i], "<title>", 7) == 0) {
				if (!append_field(new_name, af, AF_TITLE))
					return FALSE;
				i += 7;
			} else if (strncmp(&format[i], "<artist>", 8) == 0) {
				if (!append_field(new_name, af, AF_ARTIST))
					return FALSE;
				i += 8;
			} else if (strncmp(&format[i], "<album>", 7) == 0) {
				if (!append_field(new_name, af, AF_ALBUM))
					return FALSE;
				i += 7;
			} else if (strncmp(&format[i], "<year>", 6) == 0) {
				if (!append_field(new_name, af, AF_YEAR))
					return FALSE;
				i += 6;
			} else if (strncmp(&format[i], "<comment>", 9) == 0) {
				if (!append_field(new_name, af, AF_COMMENT))
					return FALSE;
				i += 9;
			} else if (strncmp(&format[i], "<track>", 7) == 0) {
				if (!append_field(new_name, af, AF_TRACK))
					return FALSE;
				i += 7;
			} else if (strncmp(&format[i], "<genre>", 7) == 0) {
				if (!append_field(new_name, af, AF_GENRE))
					return FALSE;
				i += 7;
			} else {
				g_string_append_c(new_name, format[i++]);
			}
		}
		else {
			g_string_append(new_name, &format[i]);
			break;
		}
	}

	/* add back the extension */
	g_string_append(new_name, audio_file_get_extension(af));

	return TRUE;
}


static void rename_files(GEList *file_list)
{
	GList *iter;
	audio_file *af = NULL;
	const gchar *format;
	gchar *last_full_name = "";
	gchar *orig_full_name, *orig_name, *orig_name_utf8;
	GString *new_full_name, *new_path;
	gchar *p;
	gchar *temp_utf8;
	gint new_dirs;
	gint count_total, count_renamed;
	gboolean moving;
	int res, save_errno;

	new_path = g_string_sized_new(256);
	new_full_name = g_string_sized_new(256);

	format = gtk_entry_get_text(ent_rename_format);
	moving = (strchr(format, '/') ? TRUE : FALSE);

	if (moving)
		pd_start(_("Moving Files"));
	else
		pd_start(_("Renaming Files"));
	pd_printf(PD_ICON_INFO, _("Starting in directory \"%s\""), fl_get_working_dir_utf8());

	count_total = 0;
	count_renamed = 0;
	for (iter = g_elist_first(file_list); iter; iter = g_list_next(iter)) {
		/* flush pending gtk operations so the UI doesn't freeze */
		pd_scroll_to_bottom();
		while (gtk_events_pending()) gtk_main_iteration();
		if (pd_stop_requested()) {
			pd_printf(PD_ICON_WARN, _("Operation stopped at user's request"));
			break;
		}
		
		count_total++;

		orig_full_name = (gchar *)iter->data;
		if (!fu_compare_file_paths(last_full_name, orig_full_name)) {
			temp_utf8 = str_filename_to_utf8(orig_full_name, _("(UTF8 conversion error)"));
			p = g_utf8_strrchr(temp_utf8, -1, '/');
			pd_printf(PD_ICON_INFO, _("Entering directory \"%.*s\""), (gint)(p-temp_utf8), temp_utf8);
			free(temp_utf8);
		}
		last_full_name = orig_full_name;

		/* read the file information */
		orig_name = fu_last_n_path_components(orig_full_name, 1);
		orig_name_utf8 = str_filename_to_utf8(orig_name, _("(UTF8 conversion error)"));

		res = audio_file_new(&af, orig_full_name, FALSE);
		if (res != AF_OK) {
			pd_printf(PD_ICON_FAIL, _("Error renaming \"%s\""), orig_name_utf8);

			if (res == AF_ERR_FILE)
				pd_printf(PD_ICON_NONE, _("Couldn't open file for reading"));
			else if (res == AF_ERR_FORMAT)
				pd_printf(PD_ICON_NONE, _("Audio format not recognized"));
			else 
				pd_printf(PD_ICON_NONE, _("Unknown error"));

			goto _continue;
		}

		if (!audio_file_has_tag(af)) {
			pd_printf(PD_ICON_FAIL, _("Error renaming \"%s\""), orig_name_utf8);
			pd_printf(PD_ICON_NONE, _("File has no tag"));
			goto _continue;
		}

		/* build the new file name (with path) */
		g_string_truncate(new_full_name, 0);
		if (format[0] != '/' && (p = strrchr(orig_full_name, '/'))) 
			g_string_sprintfa(new_full_name, "%.*s/", (gint)(p-orig_full_name), orig_full_name);

		if (!build_file_name(format, af, new_full_name)) {
			pd_printf(PD_ICON_FAIL, _("Error renaming \"%s\""), orig_name_utf8);
			pd_printf(PD_ICON_NONE, _("One of the tag fields is empty"));
			goto _continue;
		}
		if (strcmp(orig_full_name, new_full_name->str) == 0) {
			pd_printf(PD_ICON_OK, _("File name \"%s\" already in desired format"), orig_name_utf8);
			goto _continue;
		}

		if (fu_exists(new_full_name->str)) {
			if (moving)
				pd_printf(PD_ICON_FAIL, _("Error moving \"%s\""), orig_name_utf8);
			else
				pd_printf(PD_ICON_FAIL, _("Error renaming \"%s\""), orig_name_utf8);
			temp_utf8 = str_filename_to_utf8(new_full_name->str, _("(UTF8 conversion error)"));
			pd_printf(PD_ICON_NONE, _("File \"%s\" already exists"), temp_utf8);
			free(temp_utf8);
			goto _continue;
		}

		/* create the destination dir if necessary */
		p = strrchr(new_full_name->str, '/');
		if (moving && p) {
			new_dirs = 0;
			g_string_sprintf(new_path, "%.*s/", (gint)(p-new_full_name->str), new_full_name->str);
			res = fu_make_dir_tree(new_path->str, &new_dirs);
			if (!res) {
				save_errno = errno;
				temp_utf8 = str_filename_to_utf8(new_path->str, _("(UTF8 conversion error)"));
				pd_printf(PD_ICON_FAIL, _("Error creating directory \"%s\""), temp_utf8);
				pd_printf(PD_ICON_NONE, "%s (%d)", strerror(save_errno), save_errno);
				free(temp_utf8);
				goto _continue;
			}
			if (new_dirs) {
				pd_printf(PD_ICON_OK, _("Created directory \"%s\""), new_path->str);
			}
		}

		/* rename the file */
		res = rename(orig_full_name, new_full_name->str);
		if (res != 0) {
			save_errno = errno;
			if (moving)
				pd_printf(PD_ICON_FAIL, _("Error moving \"%s\""), orig_name_utf8);
			else
				pd_printf(PD_ICON_FAIL, _("Error renaming \"%s\""), orig_name_utf8);
			pd_printf(PD_ICON_NONE, "%s (%d)", strerror(save_errno), save_errno);
		}
		else {
			if (moving) {
				temp_utf8 = str_filename_to_utf8(new_full_name->str, _("(UTF8 conversion error)"));
				pd_printf(PD_ICON_OK, _("Moved \"%s\" to \"%s\""), orig_name_utf8, temp_utf8);
			}
			else {
				temp_utf8 = str_filename_to_utf8(g_basename(new_full_name->str), _("(UTF8 conversion error)"));
				pd_printf(PD_ICON_OK, _("Renamed \"%s\" to \"%s\""), orig_name_utf8, temp_utf8);
			}
			free(temp_utf8);
			count_renamed++;
		}

	_continue:
		if (af) {
			audio_file_delete(af);
			af = NULL;
		}
		free(orig_name_utf8);
		orig_name_utf8 = NULL;
	}

	g_string_free(new_full_name, TRUE);
	g_string_free(new_path, TRUE);

	if (moving)
		pd_printf(PD_ICON_INFO, _("Done (moved %d of %d files)"), count_renamed, count_total);
	else
		pd_printf(PD_ICON_INFO, _("Done (renamed %d of %d files)"), count_renamed, count_total);
	pd_end();

	if (count_renamed > 0)
		fl_refresh(TRUE);
}


static void start_operation()
{
	GEList *file_list;

	if (gtk_combo_box_get_active(combo_rename_apply) == APPLY_TO_ALL)
		file_list = fl_get_all_files();
	else
		file_list = fl_get_selected_files();

	if (g_elist_length(file_list) == 0) {
		pd_start(NULL);
		pd_printf(PD_ICON_FAIL, _("No files selected"));
		pd_end();

		g_elist_free(file_list);
		return;
	}

	mru_add(format_mru, gtk_entry_get_text(ent_rename_format));
	gtk_combo_set_popdown_strings(combo_rename_format, GLIST(format_mru->list));
	
	cursor_set_wait();
	gtk_widget_set_sensitive(GTK_WIDGET(b_rename_go), FALSE);
	rename_files(file_list);
	gtk_widget_set_sensitive(GTK_WIDGET(b_rename_go), TRUE);
	cursor_set_normal();

	g_elist_free(file_list);
}


/*** UI callbacks ***********************************************************/

void cb_rename_go(GtkButton *button, gpointer user_data)
{
	start_operation();
}

void cb_show_rename_chconv(GtkButton *button, gpointer user_data)
{
	chconv_display(CHCONV_RENAME);
}

void cb_rename_help(GtkButton *button, gpointer user_data)
{
	help_display(HELP_RENAME_FORMAT);
}

static void cb_file_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	if (fl_count_selected() > 0)
		gtk_combo_box_set_active(combo_rename_apply, APPLY_TO_SELECTED);
	else
		gtk_combo_box_set_active(combo_rename_apply, APPLY_TO_ALL);
}


/*** public functions *******************************************************/

void rt_init(GladeXML *xml)
{
	GtkStyle *style;
	GtkWidget *w;
	GEList *format_list;

	/*
	 * get the widgets from glade
	 */

	combo_rename_format = GTK_COMBO(glade_xml_get_widget(xml, "combo_rename_format"));
	ent_rename_format = GTK_ENTRY(glade_xml_get_widget(xml, "ent_rename_format"));
	b_rename_edit_format = GTK_BUTTON(glade_xml_get_widget(xml, "b_rename_edit_format"));
	b_rename_go = GTK_BUTTON(glade_xml_get_widget(xml, "b_rename_go"));
	combo_rename_apply = GTK_COMBO_BOX(glade_xml_get_widget(xml, "combo_rename_apply"));

	/* initialize some widgets' state */
	gtk_combo_box_set_active(combo_rename_apply, APPLY_TO_ALL);

	/* connect signals */
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_files"))),
			 "changed", G_CALLBACK(cb_file_selection_changed), NULL);


	/*
	 * set the title colors
	 */

	w = glade_xml_get_widget(xml, "lab_rename_title");
	gtk_widget_ensure_style(w);
	style = gtk_widget_get_style(w);

	gtk_widget_modify_fg(w, GTK_STATE_NORMAL, &style->text[GTK_STATE_SELECTED]);

	w = glade_xml_get_widget(xml, "box_rename_title");
	gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &style->base[GTK_STATE_SELECTED]);


	/*
	 * get the preference values, or set them to defaults
	 */

	/* format_mru */
	format_list = pref_get_ref("rt:format_mru");
	if (!format_list) {
		GEList *temp_list = g_elist_new();
		g_elist_append(temp_list, "<track>. <title>");
		g_elist_append(temp_list, "<artist> - <title>");
		g_elist_append(temp_list, "<artist> - <album>/<track>. <title>");
		g_elist_append(temp_list, "<artist>/<album>/<track>. <title>");
		g_elist_append(temp_list, "<artist>/<album> (<year>)/<track>. <title>");
		format_list = pref_set("rt:format_mru", PREF_STRING | PREF_LIST, temp_list);
		g_elist_free(temp_list);
	}
	format_mru = mru_new_from_list(10, format_list);

	gtk_combo_set_popdown_strings(combo_rename_format, GLIST(format_mru->list));
}

