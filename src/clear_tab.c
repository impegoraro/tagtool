#include <config.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "str_util.h"
#include "str_convert.h"
#include "file_util.h"
#include "file_list.h"
#include "prefs.h"
#include "progress_dlg.h"
#include "cursor.h"
#include "audio_file.h"
#ifdef ENABLE_MP3
#  include "mpeg_file.h"
#endif

#include "clear_tab.h"


enum {
	APPLY_TO_ALL = 0,
	APPLY_TO_SELECTED = 1
};


/* widgets */
static GtkButton *b_clear_go = NULL;
static GtkComboBox *combo_clear_apply = NULL;
static GtkToggleButton *rb_clear_all = NULL;
static GtkToggleButton *rb_clear_id3v1 = NULL;
static GtkToggleButton *rb_clear_id3v2 = NULL;

/* preferences */
static long* clear_option;


/*** private functions ******************************************************/

/* sets the interface state according to the preferences */
static void from_prefs()
{
	switch (*clear_option) {
		case 0: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_clear_all), TRUE);
			break;
		case 1: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_clear_id3v1), TRUE);
			break;
		case 2: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_clear_id3v2), TRUE);
			break;
	}
}

/* sets the preferences according to the curent interface state */
static void to_prefs()
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_clear_all)))
		*clear_option = 0;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_clear_id3v1)))
		*clear_option = 1;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_clear_id3v2)))
		*clear_option = 2;
}


static gboolean clear_tag(audio_file *af)
{
#ifdef ENABLE_MP3

	if (gtk_toggle_button_get_active(rb_clear_all)) {
		audio_file_remove_tag(af);
		return TRUE;
	}
	else if (af->type == AF_MPEG) {
		int version;
		if (gtk_toggle_button_get_active(rb_clear_id3v1))
			version = ID3TT_ID3V1;
		else
			version = ID3TT_ID3V2;
		mpeg_file_remove_tag_v((mpeg_file*)af, version);
		return TRUE;
	}
	else {
		return FALSE;
	}

#else

	audio_file_remove_tag(af);
	return TRUE;

#endif
}


static void clear_tags(GEList *file_list)
{
	GList *iter;
	audio_file *af;
	gchar *last_path = "";
	gchar *curr_path;
	gchar *name_utf8;
	int count_total, count_tagged;
	int save_errno, res;
	gboolean cleared;

	pd_start(_("Clearing Tags"));
	pd_printf(PD_ICON_INFO, _("Starting in directory \"%s\""), fl_get_working_dir_utf8());

	count_total = 0;
	count_tagged = 0;
	for (iter = g_elist_first(file_list); iter; iter = g_list_next(iter)) {
		/* flush pending gtk operations so the UI doesn't freeze */
		pd_scroll_to_bottom();
		while (gtk_events_pending()) gtk_main_iteration();
		if (pd_stop_requested()) {
			pd_printf(PD_ICON_WARN, _("Operation stopped at user's request"));
			break;
		}

		count_total++;

		curr_path = (gchar *)iter->data;

		if (!fu_compare_file_paths(last_path, curr_path)) {
			gchar *p;
			gchar *utf8 = str_filename_to_utf8(curr_path, _("(UTF8 conversion error)"));
			p = g_utf8_strrchr(utf8, -1, '/');

			pd_printf(PD_ICON_INFO, _("Entering directory \"%.*s\""), (gint)(p-utf8), utf8);
			free(utf8);
		}
		last_path = curr_path;

		name_utf8 = str_filename_to_utf8(g_basename(curr_path), _("(UTF8 conversion error)"));

		res = audio_file_new(&af, curr_path, TRUE);
		if (res != AF_OK) {
			pd_printf(PD_ICON_FAIL, _("Error in file \"%s\""), name_utf8);
			if (res == AF_ERR_FILE)
				pd_printf(PD_ICON_NONE, _("Couldn't open file for writing"));
			else if (res == AF_ERR_FORMAT)
				pd_printf(PD_ICON_NONE, _("Audio format not recognized"));
			else 
				pd_printf(PD_ICON_NONE, _("Unknown error (%d)"), res);

			goto _continue;
		}

		cleared = clear_tag(af);
		if (!cleared) {
			pd_printf(PD_ICON_WARN, _("Skipped \"%s\""), name_utf8);
			goto _continue;
		}

		res = audio_file_write_changes(af);
		if (res != AF_OK) {
			save_errno = errno;
			pd_printf(PD_ICON_FAIL, _("Error in file \"%s\""), name_utf8);
			pd_printf(PD_ICON_NONE, "%s (%d)", strerror(save_errno), save_errno);

			goto _continue;
		}

		pd_printf(PD_ICON_OK, _("Cleared tag from \"%s\""), name_utf8);
		count_tagged++;

	_continue:
		if (af) {
			audio_file_delete(af);
			af = NULL;
		}
		free(name_utf8);
		name_utf8 = NULL;
	}

	pd_printf(PD_ICON_INFO, _("Done (Cleared %d of %d files)"), count_tagged, count_total);
	pd_end();
}


/* orchestrates the call to clear_tags() */
static void start_operation()
{
	GEList *file_list;

	/*
	int button;
	button = message_box(w_main, "Remove all tags", 
			     "This will remove tags from all selected files. Proceed?", 0, 
			     GTK_STOCK_CANCEL, GTK_STOCK_YES, NULL);
	if (button == 0)
		return;
	*/

	if (gtk_combo_box_get_active(combo_clear_apply) == APPLY_TO_ALL)
		file_list = fl_get_all_files();
	else
		file_list = fl_get_selected_files();

	if (g_elist_length(file_list) == 0) {
		pd_start(_("Clearing Tags"));
		pd_printf(PD_ICON_FAIL, _("No files selected"));
		pd_end();

		g_elist_free(file_list);
		return;
	}

	to_prefs();
	
	cursor_set_wait();
	gtk_widget_set_sensitive(GTK_WIDGET(b_clear_go), FALSE);
	clear_tags(file_list);
	gtk_widget_set_sensitive(GTK_WIDGET(b_clear_go), TRUE);
	cursor_set_normal();

	g_elist_free(file_list);
}


/*** UI callbacks ***********************************************************/

void cb_clear_go(GtkButton *button, gpointer user_data)
{
	start_operation();
}

static void cb_file_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	if (fl_count_selected() > 0)
		gtk_combo_box_set_active(combo_clear_apply, APPLY_TO_SELECTED);
	else
		gtk_combo_box_set_active(combo_clear_apply, APPLY_TO_ALL);
}


/*** public functions *******************************************************/

void ct_init(GladeXML *xml)
{
	GtkStyle *style;
	GtkWidget *w;

	/*
	 * get the widgets from glade
	 */

	b_clear_go = GTK_BUTTON(glade_xml_get_widget(xml, "b_clear_go"));
	combo_clear_apply = GTK_COMBO_BOX(glade_xml_get_widget(xml, "combo_clear_apply"));

	rb_clear_all = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_clear_all"));
	rb_clear_id3v1 = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_clear_id3v1"));
	rb_clear_id3v2 = GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "rb_clear_id3v2"));

	/* initialize some widgets' state */
	gtk_combo_box_set_active(combo_clear_apply, APPLY_TO_ALL);

	/* connect signals */
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_files"))),
			 "changed", G_CALLBACK(cb_file_selection_changed), NULL);

#ifndef ENABLE_MP3
	gtk_widget_hide(glade_xml_get_widget(xml, "tab_clear_options"));
#endif


	/*
	 * set the title colors
	 */

	w = glade_xml_get_widget(xml, "lab_clear_title");
	gtk_widget_ensure_style(w);
	style = gtk_widget_get_style(w);

	gtk_widget_modify_fg(w, GTK_STATE_NORMAL, &style->text[GTK_STATE_SELECTED]);

	w = glade_xml_get_widget(xml, "box_clear_title");
	gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &style->base[GTK_STATE_SELECTED]);


	/*
	 * get the preference values, or set them to defaults
	 */

	/* clear_option */
	clear_option = pref_get_ref("ct:clear_option");
	if (clear_option == NULL) {
		long def = 0;
		clear_option = pref_set("ct:clear_option", PREF_INT, &def);
	}

	from_prefs();
}

