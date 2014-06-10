#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "elist.h"
#include "str_convert.h"
#include "main_win.h"
#include "file_list.h"
#include "status_bar.h"
#include "audio_file.h"
#include "edit_tab.h"


/* widgets */
static GtkNotebook *nb_file = NULL;
static GtkNotebook *nb_edit = NULL;
static GtkLabel *lab_info_names = NULL;
static GtkLabel *lab_info_values = NULL;
static GtkWidget *align_create_tag = NULL;
static GtkWidget *align_remove_tag = NULL;

/* private data */
static audio_file *af = NULL;


/*** UI callbacks ***********************************************************/

void cb_notebook_switch(GtkNotebook *nb, GtkNotebookPage *page, gint page_num, gpointer user_data)
{
	static gint last = 0;

	if (last == 0 && page_num != 0) {
		/* when leaving edit tab */
		et_unload_file();
	}
	else if (last != 0 && page_num == 0) {
		/* when entering edit tab */
		if (fl_count_selected() == 1)
			et_load_file(fl_get_selected_file());
	}

	last = page_num;
}


/*** public functions *******************************************************/

void et_init(GladeXML *xml)
{
	GtkStyle *style;
	GtkWidget *w;

	/* widgets and icons */
	nb_file = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_file"));
	nb_edit = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_edit"));
	align_create_tag = glade_xml_get_widget(xml, "align_create_tag");
	align_remove_tag = glade_xml_get_widget(xml, "align_remove_tag");
	lab_info_names = GTK_LABEL(glade_xml_get_widget(xml, "lab_info_names"));
	lab_info_values = GTK_LABEL(glade_xml_get_widget(xml, "lab_info_values"));

	/* set the title colors */
	w = glade_xml_get_widget(xml, "lab_edit_title");
	gtk_widget_ensure_style(w);
	style = gtk_widget_get_style(w);

	gtk_widget_modify_fg(w, GTK_STATE_NORMAL, &style->text[GTK_STATE_SELECTED]);

	w = glade_xml_get_widget(xml, "box_edit_title");
	gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &style->base[GTK_STATE_SELECTED]);
}


void et_load_file(const gchar *name)
{
	GString *info_names;
	GString *info_values;
	gchar *name_utf8;
	gchar *buf, *aux, *tok;
	int res;

	/* in some cases the previous file may still be loaded */
	if (af) {
		audio_file_delete(af);
		af = NULL;
	}

	/* create the audio_file object */
	res = audio_file_new(&af, name, TRUE);
	if (res == AF_ERR_FILE)
		res = audio_file_new(&af, name, FALSE);

	if (res != AF_OK) {
		name_utf8 = str_filename_to_utf8(name, _("(UTF8 conversion error)"));

		if (res == AF_ERR_FILE)
			sb_printf(_("Couldn't open file %s"), name_utf8);
		else if (res == AF_ERR_FORMAT)
			sb_printf(_("Audio file format not recognized: %s"), name_utf8);
		else 
			sb_printf(_("Unknow error when opening file %s"), name_utf8);

		free(name_utf8);
		return;
	}

	if (audio_file_is_editable(af))
		sb_clear();
	else {
		name_utf8 = str_filename_to_utf8(name, _("(UTF8 conversion error)"));
		sb_printf(_("File is read-only: %s"), name);
		free(name_utf8);
	}

	
	/* fill in the File Info box */
	info_names = g_string_sized_new(128);
	info_values = g_string_sized_new(128);
	
	g_string_append(info_names, _("File Type:"));
	g_string_append(info_values, audio_file_get_desc(af));

	buf = strdup(audio_file_get_info(af));
	aux = buf;
	while(1) {
		tok = strtok(aux, "\n");
		if (!tok) break;
		g_string_sprintfa(info_names, "\n%s:", tok);
		if (aux) aux = NULL;

		tok = strtok(aux, "\n");
		if (!tok) break;
		g_string_sprintfa(info_values, "\n%s", tok);
	}

	gtk_label_set_text(lab_info_names, info_names->str);
	gtk_label_set_text(lab_info_values, info_values->str);

	free(buf);
	g_string_free(info_names, TRUE);
	g_string_free(info_values, TRUE);


	/* display the appropriate edit interface */
	audio_file_edit_load(af);

	gtk_notebook_set_page(nb_file, 1);
}


void et_unload_file()
{
	if (af) {
		audio_file *temp = af;
		af = NULL;
		audio_file_edit_unload(temp);
		audio_file_delete(temp);

		gtk_notebook_set_page(nb_file, 0);
	}
}

