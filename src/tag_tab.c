#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#ifdef HAVE_GNU_REGEX_H
# include <gnu/regex.h>	/* for recent FreeBSD */
#elif HAVE_GNUREGEX_H
# include <gnuregex.h>	/* for older FreeBSD */
#else
# include <regex.h>	/* for everyone else */
#endif

#include "elist.h"
#include "file_util.h"
#include "str_util.h"
#include "str_convert.h"
#include "mru.h"
#include "prefs.h"
#include "audio_file.h"
#include "genre.h"
#include "file_list.h"
#include "char_conv_dlg.h"
#include "message_box.h"
#include "progress_dlg.h"
#include "cursor.h"
#include "help.h"
#include "tag_tab.h"


enum {
	APPLY_TO_ALL = 0,
	APPLY_TO_SELECTED = 1
};


/* for parsing file names */
typedef struct {
	/* regexp to match against the file name */
	regex_t re;
	/* indexes of substring matches */
	gint title;
	gint artist;
	gint album;
	gint year;
	gint comment;
	gint track;
	gint genre;
} parse_info;


/* widgets */
static GtkCombo *combo_tag_format = NULL;
static GtkEntry *ent_tag_format = NULL;
static GtkButton *b_tag_edit_format = NULL;
static GtkCheckButton *cb_use_filename = NULL;
static GtkButton *b_tag_go = NULL;
static GtkComboBox *combo_tag_apply = NULL;
static GtkCheckButton *cb_title = NULL;
static GtkCheckButton *cb_artist = NULL;
static GtkCheckButton *cb_album = NULL;
static GtkCheckButton *cb_year = NULL;
static GtkCheckButton *cb_comment = NULL;
static GtkCheckButton *cb_track = NULL;
static GtkCheckButton *cb_genre = NULL;
static GtkEntry *ent_title = NULL;
static GtkEntry *ent_artist = NULL;
static GtkEntry *ent_album = NULL;
static GtkEntry *ent_year = NULL;
static GtkEntry *ent_comment = NULL;
static GtkEntry *ent_track = NULL;
static GtkCheckButton *cb_track_auto = NULL;
static GtkCombo *combo_genre = NULL;
static GtkEntry *ent_genre = NULL;
static GtkWindow *w_main = NULL;

/* private data */
static gboolean ignore_field_changed = FALSE;
static gboolean from_fn_title = FALSE;
static gboolean from_fn_artist = FALSE;
static gboolean from_fn_album = FALSE;
static gboolean from_fn_year = FALSE;
static gboolean from_fn_comment = FALSE;
static gboolean from_fn_track = FALSE;
static gboolean from_fn_genre = FALSE;

/* preferences */
static gboolean *use_filename;
static MRUList *format_mru;


/*** private functions ******************************************************/

static parse_info *build_parse_info(const gchar *format)
{
	parse_info *info = calloc(1, sizeof(parse_info));
	gchar *aux_str = calloc(1, 2 + 2*strlen(format));
	gchar *expr_str = calloc(1, 2 + 2*strlen(format));
	gint i, j, span, pos;
	gchar *p;
	gint res;


	/* printf("format: %s\n", format); */

	/* constrain the expression to match the whole string and escape
	   characters that have special meaning in regexps */
	i = 0;
	j = 0;
	aux_str[j++] = '^';
	while (format[i]) {
		if (strchr(".*+?[]{}()|^$\\", format[i]))
			aux_str[j++] = '\\';
		aux_str[j++] = format[i++];
	}
	aux_str[j++] = '$';
	aux_str[j++] = 0;

	/* find the markers, record their relative positions and replace 
	   them in the final expression */
	i = 0;
	j = 0;
	pos = 1;
	while (TRUE) {
		p = index(&aux_str[i], '<');
		if (p != NULL) {
			span = (gint)(p - &aux_str[i]);
			if (span > 0) {
				strncpy(&expr_str[j], &aux_str[i], span);
				i += span;
				j += span;
			} else if (strncmp(&aux_str[i], "<title>", 7) == 0) {
				strncpy(&expr_str[j], "(.*)", 4);
				info->title = pos++;
				i += 7;
				j += 4;
			} else if (strncmp(&aux_str[i], "<artist>", 8) == 0) {
				strncpy(&expr_str[j], "(.*)", 4);
				info->artist = pos++;
				i += 8;
				j += 4;
			} else if (strncmp(&aux_str[i], "<album>", 7) == 0) {
				strncpy(&expr_str[j], "(.*)", 4);
				info->album = pos++;
				i += 7;
				j += 4;
			} else if (strncmp(&aux_str[i], "<year>", 6) == 0) {
				strncpy(&expr_str[j], "(.*)", 4);
				info->year = pos++;
				i += 6;
				j += 4;
			} else if (strncmp(&aux_str[i], "<comment>", 9) == 0) {
				strncpy(&expr_str[j], "(.*)", 4);
				info->comment = pos++;
				i += 9;
				j += 4;
			} else if (strncmp(&aux_str[i], "<track>", 7) == 0) {
				strncpy(&expr_str[j], "([^ ]*)", 7);
				info->track = pos++;
				i += 7;
				j += 7;
			} else if (strncmp(&aux_str[i], "<genre>", 7) == 0) {
				strncpy(&expr_str[j], "(.*)", 4);
				info->genre = pos++;
				i += 7;
				j += 4;
			} else if (strncmp(&aux_str[i], "<\\*>", 4) == 0) {
				strncpy(&expr_str[j], ".*", 2);
				i += 4;
				j += 2;
			} else {
				expr_str[j++] = aux_str[i++];
			}
		} else {
			strcpy(&expr_str[j], &aux_str[i]);
			break;
		}
	}


	/* printf("regexp: %s\n", expr_str); */

	/* compile the regexp from the expression string */
	res = regcomp(&info->re, expr_str, REG_EXTENDED | REG_ICASE);
	if (res != 0) {
		g_warning("error in regcomp (\"%s\"): %d", expr_str, res);
		free(aux_str);
		free(expr_str);
		free(info);
		return NULL;
	}

	free(aux_str);
	free(expr_str);

	return info;
}

static void free_parse_info(parse_info *info)
{
	regfree(&info->re);
	free(info);
}


static void set_field(audio_file *af, int field, gchar *src, int len)
{
	chconv_tag_options options;
	char *temp;
	char *buf = malloc(len+1);
	str_safe_strncpy(buf, src, len+1);

	/* apply character conversions */
	options = chconv_get_tag_options();
	if (options.space_conv != NULL && options.space_conv[0] != 0) {
		str_rtrim(buf);

		temp = buf;
		buf = str_replace_char(buf, options.space_conv[0], ' ');
		free(temp);
	}
	if (options.case_conv != CASE_CONV_NONE) {
		temp = buf;
		buf = str_convert_case(buf, options.case_conv);
		free(temp);
	}

	audio_file_set_field(af, field, buf);
	free(buf);
}

static gboolean tag_from_filename(parse_info *pi, gchar *filename, audio_file *af)
{
	regmatch_t matches[8];
	char *filename_utf8;
	char *p;
	int res;

	/* convert to UTF-8 */
	filename_utf8 = str_filename_to_utf8(filename, NULL);
	if (filename_utf8 == NULL)
		return FALSE;

	/* truncate the file extension */
	p = g_utf8_strrchr(filename_utf8, -1, '.');
	if (p && strlen(p) <= 5)
		*p = 0;

	/* printf("string: %s\n", filename_utf8); */

	
	/* match the regexp against the file name (minus extension) and 
	   extract the tag fields */
	res = regexec(&pi->re, filename_utf8, 8, matches, 0);
	if (res != 0) {
		free(filename_utf8);
		return FALSE;
	}

	if (pi->title > 0) {
		set_field(af, AF_TITLE, &filename_utf8[matches[pi->title].rm_so], 
			  matches[pi->title].rm_eo - matches[pi->title].rm_so);
	}
	if (pi->artist > 0) {
		set_field(af, AF_ARTIST, &filename_utf8[matches[pi->artist].rm_so], 
			  matches[pi->artist].rm_eo - matches[pi->artist].rm_so);
	}
	if (pi->album > 0) {
		set_field(af, AF_ALBUM, &filename_utf8[matches[pi->album].rm_so], 
			  matches[pi->album].rm_eo - matches[pi->album].rm_so);
	}
	if (pi->year > 0) {
		set_field(af, AF_YEAR, &filename_utf8[matches[pi->year].rm_so], 
			  matches[pi->year].rm_eo - matches[pi->year].rm_so);
	}
	if (pi->comment > 0) {
		set_field(af, AF_COMMENT, &filename_utf8[matches[pi->comment].rm_so], 
			  matches[pi->comment].rm_eo - matches[pi->comment].rm_so);
	}
	if (pi->track > 0) {
		set_field(af, AF_TRACK, &filename_utf8[matches[pi->track].rm_so], 
			  matches[pi->track].rm_eo - matches[pi->track].rm_so);
	}
	if (pi->genre > 0) {
		set_field(af, AF_GENRE, &filename_utf8[matches[pi->genre].rm_so], 
			  matches[pi->genre].rm_eo - matches[pi->genre].rm_so);
	}

	free(filename_utf8);
	return TRUE;
}


static void tag_files(GEList *file_list)
{
	GList *iter;
	audio_file *af;
	parse_info *pi;
	gint path_components;
	gchar *last_path;
	gchar *curr_path;
	gchar *file_name = NULL;
	gchar *file_name_utf8 = NULL;
	gchar *temp_utf8;
	gint track_auto_index;
	gint count_total, count_tagged;
	int save_errno, res;

	gboolean write_title   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_title));
	gboolean write_artist  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_artist));
	gboolean write_album   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_album));
	gboolean write_year    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_year));
	gboolean write_comment = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_comment));
	gboolean write_track   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_track));
	gboolean write_genre   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_genre));
	gboolean track_auto    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_track_auto));

	pd_start(_("Tagging Files"));
	if (!(write_title || write_artist || write_album || write_year || 
	      write_comment || write_track || write_genre))
	{
		pd_printf(PD_ICON_INFO, _("No tag fields to set!"));
		pd_end();
		return;
	}
	pd_printf(PD_ICON_INFO, _("Starting in directory \"%s\""), fl_get_working_dir_utf8());

	if (*use_filename) {
		const gchar *format = gtk_entry_get_text(ent_tag_format);
		pi = build_parse_info(format);
		path_components = fu_count_path_components(format);
	}
	else {
		pi = NULL;
		path_components = 0;
	}

	count_total = 0;
	count_tagged = 0;
	track_auto_index = 1;
	last_path = "";
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
			temp_utf8 = str_filename_to_utf8(curr_path, _("(UTF8 conversion error)"));
			p = g_utf8_strrchr(temp_utf8, -1, '/');
			pd_printf(PD_ICON_INFO, _("Entering directory \"%.*s\""), (gint)(p-temp_utf8), temp_utf8);
			free(temp_utf8);

			track_auto_index = 1;	/* new dir, reset track auto-increment */
		}
		last_path = curr_path;

		file_name = fu_last_n_path_components(curr_path, 1);
		file_name_utf8 = str_filename_to_utf8(file_name, _("(UTF8 conversion error)"));

		res = audio_file_new(&af, curr_path, TRUE);
		if (res != AF_OK) {
			pd_printf(PD_ICON_FAIL, _("Error tagging \"%s\""), file_name_utf8);

			if (res == AF_ERR_FILE)
				pd_printf(PD_ICON_NONE, _("Couldn't open file for writing"));
			else if (res == AF_ERR_FORMAT)
				pd_printf(PD_ICON_NONE, _("Audio format not recognized"));
			else 
				pd_printf(PD_ICON_NONE, _("Unknown error (%d)"), res);

			goto _continue;
		}

		/* create the tag if not already present */
		audio_file_create_tag(af);
	
		/* fill in the values from the form */
		if (write_title && !(from_fn_title && *use_filename))
			audio_file_set_field(af, AF_TITLE, gtk_entry_get_text(ent_title));
		if (write_artist && !(from_fn_artist && *use_filename))
			audio_file_set_field(af, AF_ARTIST, gtk_entry_get_text(ent_artist));
		if (write_album && !(from_fn_album && *use_filename))
			audio_file_set_field(af, AF_ALBUM, gtk_entry_get_text(ent_album));
		if (write_year && !(from_fn_year && *use_filename))
			audio_file_set_field(af, AF_YEAR, gtk_entry_get_text(ent_year));
		if (write_comment && !(from_fn_comment && *use_filename))
			audio_file_set_field(af, AF_COMMENT, gtk_entry_get_text(ent_comment));
		if (write_track && !(from_fn_track && *use_filename)) {
			if (track_auto) {
				char buf[10];
				snprintf(buf, 10, "%02u", track_auto_index++);
				audio_file_set_field(af, AF_TRACK, buf);
			} else 
				audio_file_set_field(af, AF_TRACK, gtk_entry_get_text(ent_track));
		}
		if (write_genre && !(from_fn_genre && *use_filename))
			audio_file_set_field(af, AF_GENRE, gtk_entry_get_text(GTK_ENTRY(combo_genre->entry)));

		/* fill in the values from the file name */
		if (pi != NULL) {
			gchar *full_path = fu_join_path(fl_get_working_dir(), curr_path);
			gchar *relevant_path = fu_last_n_path_components(full_path, path_components);

			res = tag_from_filename(pi, relevant_path, af);
			free(full_path);

			if (!res) {
				pd_printf(PD_ICON_FAIL, _("Error tagging \"%s\""), file_name_utf8);
				pd_printf(PD_ICON_NONE, _("File name does not match expected format"));
				goto _continue;
			}
		}

		/* write new tag to file */
		res = audio_file_write_changes(af);
		if (res != AF_OK) {
			save_errno = errno;
			pd_printf(PD_ICON_FAIL, _("Error tagging \"%s\""), file_name_utf8);

			if (res == AF_ERR_FILE)
				pd_printf(PD_ICON_NONE, "%s (%d)", strerror(save_errno), save_errno);
			else 
				pd_printf(PD_ICON_NONE, _("Unknown error (%d)"), res);

			goto _continue;
		}

		pd_printf(PD_ICON_OK, _("Tagged \"%s\""), file_name_utf8);
		count_tagged++;

	_continue:
		free(file_name_utf8);
		if (af) {
			audio_file_delete(af);
			af = NULL;
		}
	}

	pd_printf(PD_ICON_INFO, _("Done (tagged %d of %d files)"), count_tagged, count_total);
	pd_end();

	if (pi != NULL)
		free_parse_info(pi);
}


static void start_operation()
{
	GEList *file_list;

	if (gtk_combo_box_get_active(combo_tag_apply) == APPLY_TO_ALL)
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

	cursor_set_wait();
	gtk_widget_set_sensitive(GTK_WIDGET(b_tag_go), FALSE);

	mru_add(format_mru, gtk_entry_get_text(ent_tag_format));
	gtk_combo_set_popdown_strings(combo_tag_format, GLIST(format_mru->list));

	tag_files(file_list);

	gtk_widget_set_sensitive(GTK_WIDGET(b_tag_go), TRUE);
	cursor_set_normal();

	g_elist_free(file_list);
}


static void update_interface_fname()
{
	static gboolean last_title = FALSE;
	static gboolean last_artist = FALSE;
	static gboolean last_album = FALSE;
	static gboolean last_year = FALSE;
	static gboolean last_comment = FALSE;
	static gboolean last_track = FALSE;
	static gboolean last_genre = FALSE;

	const gchar *format = gtk_entry_get_text(ent_tag_format);
	gboolean value;
	gboolean save;

	/* inhibit proccessing signals in 'cb_filed_changed' */
	save = ignore_field_changed;
	ignore_field_changed = TRUE;

	if (strstr(format, "<title>")) from_fn_title = TRUE;
	else from_fn_title = FALSE;
	if (strstr(format, "<artist>")) from_fn_artist = TRUE;
	else from_fn_artist = FALSE;
	if (strstr(format, "<album>")) from_fn_album = TRUE;
	else from_fn_album = FALSE;
	if (strstr(format, "<year>")) from_fn_year = TRUE;
	else from_fn_year = FALSE;
	if (strstr(format, "<comment>")) from_fn_comment = TRUE;
	else from_fn_comment = FALSE;
	if (strstr(format, "<track>")) from_fn_track = TRUE;
	else from_fn_track = FALSE;
	if (strstr(format, "<genre>")) from_fn_genre = TRUE;
	else from_fn_genre = FALSE;

	value = *use_filename && from_fn_title;
	if (last_title != value) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_title), value);
		gtk_entry_set_text(ent_title, (value ? "<from filename>" : ""));
		gtk_widget_set_sensitive(GTK_WIDGET(cb_title), !value);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_title), !value);
		last_title = value;
	}
	value = *use_filename && from_fn_artist;
	if (last_artist != value) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_artist), value);
		gtk_entry_set_text(ent_artist, (value ? "<from filename>" : ""));
		gtk_widget_set_sensitive(GTK_WIDGET(cb_artist), !value);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_artist), !value);
		last_artist = value;
	}
	value = *use_filename && from_fn_album;
	if (last_album != value) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_album), value);
		gtk_entry_set_text(ent_album, (value ? "<from filename>" : ""));
		gtk_widget_set_sensitive(GTK_WIDGET(cb_album), !value);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_album), !value);
		last_album = value;
	}
	value = *use_filename && from_fn_year;
	if (last_year != value) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_year), value);
		gtk_entry_set_text(ent_year, (value ? "<fn>" : ""));
		gtk_widget_set_sensitive(GTK_WIDGET(cb_year), !value);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_year), !value);
		last_year = value;
	}
	value = *use_filename && from_fn_comment;
	if (last_comment != value) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_comment), value);
		gtk_entry_set_text(ent_comment, (value ? "<from filename>" : ""));
		gtk_widget_set_sensitive(GTK_WIDGET(cb_comment), !value);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_comment), !value);
		last_comment = value;
	}
	value = *use_filename && from_fn_track;
	if (last_track != value) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_track), value);
		gtk_entry_set_text(ent_track, (value ? "<fn>" : ""));
		gtk_widget_set_sensitive(GTK_WIDGET(cb_track), !value);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_track), !value);
		if (value)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_track_auto), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(cb_track_auto), !value);
		last_track = value;
	}
	value = *use_filename && from_fn_genre;
	if (last_genre != value) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_genre), value);
		gtk_entry_set_text(ent_genre, (value ? "<from filename>" : ""));
		gtk_widget_set_sensitive(GTK_WIDGET(cb_genre), !value);
		gtk_widget_set_sensitive(GTK_WIDGET(combo_genre), !value);
		last_genre = value;
	}

	ignore_field_changed = save;
}


/*** UI callbacks ***********************************************************/

void cb_tag_go(GtkButton *button, gpointer user_data)
{
	start_operation();
}

void cb_update_filename(GObject *obj, gpointer data)
{
	*use_filename = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_use_filename));
	update_interface_fname();
}

void cb_field_changed(GObject *obj, gpointer data)
{
	if (ignore_field_changed)
		return;

	if ((void*)obj == (void*)ent_title)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_title), TRUE);
	else if ((void*)obj == (void*)ent_artist)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_artist), TRUE);
	else if ((void*)obj == (void*)ent_album)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_album), TRUE);
	else if ((void*)obj == (void*)ent_year)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_year), TRUE);
	else if ((void*)obj == (void*)ent_genre)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_genre), TRUE);
	else if ((void*)obj == (void*)ent_comment)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_comment), TRUE);
	else if ((void*)obj == (void*)ent_track || (void*)obj == (void*)cb_track_auto)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_track), TRUE);
}

void cb_check_changed(GtkToggleButton *widget, gpointer data)
{
	gboolean save;
	
	if (gtk_toggle_button_get_active(widget))
		return;

	/* inhibit proccessing signals in 'cb_field_changed' */
	save = ignore_field_changed;
	ignore_field_changed = TRUE;

	if ((void*)widget == (void*)cb_title)
		gtk_entry_set_text(ent_title, "");
	else if ((void*)widget == (void*)cb_artist)
		gtk_entry_set_text(ent_artist, "");
	else if ((void*)widget == (void*)cb_album)
		gtk_entry_set_text(ent_album, "");
	else if ((void*)widget == (void*)cb_year)
		gtk_entry_set_text(ent_year, "");
	else if ((void*)widget == (void*)cb_genre)
		gtk_entry_set_text(ent_genre, "");
	else if ((void*)widget == (void*)cb_comment)
		gtk_entry_set_text(ent_comment, "");
	else if ((void*)widget == (void*)cb_track) {
		gtk_entry_set_text(ent_track, "");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_track_auto), FALSE);
	}

	ignore_field_changed = save;
}

void cb_show_tag_chconv(GtkButton *button, gpointer user_data)
{
	chconv_display(CHCONV_TAG);
}

void cb_tag_help(GtkButton *button, gpointer user_data)
{
	help_display(HELP_TAG_FORMAT);
}

static void cb_file_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	if (fl_count_selected() > 0)
		gtk_combo_box_set_active(combo_tag_apply, APPLY_TO_SELECTED);
	else
		gtk_combo_box_set_active(combo_tag_apply, APPLY_TO_ALL);
}


/*** public functions *******************************************************/

void tt_init(GladeXML *xml)
{
	GtkStyle *style;
	GtkWidget *w;
	GEList *genre_list;
	GEList *format_list;

	/*
	 * get the widgets from glade
	 */

	combo_tag_format = GTK_COMBO(glade_xml_get_widget(xml, "combo_tag_format"));
	ent_tag_format = GTK_ENTRY(glade_xml_get_widget(xml, "ent_tag_format"));
	b_tag_edit_format = GTK_BUTTON(glade_xml_get_widget(xml, "b_tag_edit_format"));
	cb_use_filename = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_use_filename"));
	b_tag_go = GTK_BUTTON(glade_xml_get_widget(xml, "b_tag_go"));
	combo_tag_apply = GTK_COMBO_BOX(glade_xml_get_widget(xml, "combo_tag_apply"));

	cb_title = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_title"));
	cb_artist = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_artist"));
	cb_album = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_album"));
	cb_year = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_year"));
	cb_comment = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_comment"));
	cb_track = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_track"));
	cb_genre = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_genre"));

	ent_title = GTK_ENTRY(glade_xml_get_widget(xml, "ent_title2"));
	ent_artist = GTK_ENTRY(glade_xml_get_widget(xml, "ent_artist2"));
	ent_album = GTK_ENTRY(glade_xml_get_widget(xml, "ent_album2"));
	ent_year = GTK_ENTRY(glade_xml_get_widget(xml, "ent_year2"));
	ent_comment = GTK_ENTRY(glade_xml_get_widget(xml, "ent_comment2"));
	ent_track = GTK_ENTRY(glade_xml_get_widget(xml, "ent_track2"));
	cb_track_auto = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_track_auto"));
	combo_genre = GTK_COMBO(glade_xml_get_widget(xml, "combo_genre2"));
	ent_genre = GTK_ENTRY(glade_xml_get_widget(xml, "ent_genre2"));

	w_main = GTK_WINDOW(glade_xml_get_widget(xml, "w_main"));

	/* initialize some widgets' state */
	gtk_combo_box_set_active(combo_tag_apply, APPLY_TO_ALL);

	genre_list = genre_create_list(TRUE);
	g_elist_prepend(genre_list, "");
	gtk_combo_set_popdown_strings(combo_genre, GLIST(genre_list));
	g_elist_free(genre_list);

	/* connect signals */
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(xml, "tv_files"))),
			 "changed", G_CALLBACK(cb_file_selection_changed), NULL);


	/*
	 * set the title colors
	 */

	w = glade_xml_get_widget(xml, "lab_tag_title");
	gtk_widget_ensure_style(w);
	style = gtk_widget_get_style(w);

	gtk_widget_modify_fg(w, GTK_STATE_NORMAL, &style->text[GTK_STATE_SELECTED]);

	w = glade_xml_get_widget(xml, "box_tag_title");
	gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &style->base[GTK_STATE_SELECTED]);


	/*
	 * get the preference values, or set them to defaults
	 */

	/* use_filename */
	use_filename = pref_get_ref("tt:use_filename");
	if (!use_filename) {
		gboolean temp = TRUE;
		use_filename = pref_set("tt:use_filename", PREF_BOOLEAN, &temp);
	}

	/* format_mru */
	format_list = pref_get_ref("tt:format_mru");
	if (!format_list) {
		GEList *temp_list = g_elist_new();
		g_elist_append(temp_list, "<track>. <title>");
		g_elist_append(temp_list, "<artist> - <title>");
		g_elist_append(temp_list, "<artist> - <album>/<track>. <title>");
		g_elist_append(temp_list, "<artist>/<album>/<track>. <title>");
		g_elist_append(temp_list, "<artist>/<album> (<year>)/<track>. <title>");
		format_list = pref_set("tt:format_mru", PREF_STRING | PREF_LIST, temp_list);
		g_elist_free(temp_list);
	}
	format_mru = mru_new_from_list(10, format_list);


	/*
	 * synchronize the interface state
	 */

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_use_filename), *use_filename);
	gtk_combo_set_popdown_strings(combo_tag_format, GLIST(format_mru->list));

	update_interface_fname();
}

