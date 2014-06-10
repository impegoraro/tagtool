#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "gtk_util.h"
#include "prefs.h"

#include "char_conv_dlg.h"


/* options */
static gboolean* tag_space_conv;
static char*     tag_space_conv_chars;
static int*      tag_case_conv;

static gboolean* rename_space_conv;
static char*     rename_space_conv_chars;
static gboolean* rename_invalid_conv;
static char*     rename_invalid_conv_chars;
static int*      rename_case_conv;


/* widgets */
static GtkWindow *dlg_char_conv = NULL;
static GtkNotebook *nb_char_conv = NULL;

static GtkRadioButton *rb_t_conv_space_none = NULL;
static GtkRadioButton *rb_t_conv_space_from = NULL;
static GtkEntry *ent_t_conv_chars = NULL;
static GtkComboBox *combo_t_case = NULL;

static GtkRadioButton *rb_r_conv_space_none = NULL;
static GtkRadioButton *rb_r_conv_space_to = NULL;
static GtkRadioButton *rb_r_invalid_omit = NULL;
static GtkRadioButton *rb_r_invalid_convert = NULL;
static GtkEntry *ent_r_conv_chars = NULL;
static GtkEntry *ent_r_invalid_chars = NULL;
static GtkComboBox *combo_r_case = NULL;


/*** private functions ******************************************************/

/* sets the interface state according to the preferences */
static void from_prefs()
{
	/* tag tab */
	if (*tag_space_conv) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_t_conv_space_from), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_t_conv_chars), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_t_conv_space_none), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_t_conv_chars), FALSE);
	}
	gtk_entry_set_text(ent_t_conv_chars, tag_space_conv_chars);

	gtk_combo_box_set_active(combo_t_case, *tag_case_conv);

	/* rename tab */
	if (*rename_space_conv) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_r_conv_space_to), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_r_conv_chars), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_r_conv_space_none), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_r_conv_chars), FALSE);
	}
	gtk_entry_set_text(ent_r_conv_chars, rename_space_conv_chars);

	if (*rename_invalid_conv) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_r_invalid_convert), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_r_invalid_chars), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_r_invalid_omit), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(ent_r_invalid_chars), FALSE);
	}
	gtk_entry_set_text(ent_r_invalid_chars, rename_invalid_conv_chars);

	gtk_combo_box_set_active(combo_r_case, *rename_case_conv);
}

/* sets the preferences according to the curent interface state */
static void to_prefs()
{
	/* tag tab */
	*tag_space_conv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_t_conv_space_from));
	tag_space_conv_chars = pref_set("chconv:tag_space_conv_chars", PREF_STRING, 
					(void*)gtk_entry_get_text(ent_t_conv_chars));

	*tag_case_conv = gtk_combo_box_get_active(combo_t_case);

	/* rename tab */
	*rename_space_conv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_r_conv_space_to));
	rename_space_conv_chars = pref_set("chconv:rename_space_conv_chars", PREF_STRING, 
					   (void*)gtk_entry_get_text(ent_r_conv_chars));

	*rename_invalid_conv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_r_invalid_convert));
	rename_invalid_conv_chars = pref_set("chconv:rename_invalid_conv_chars", PREF_STRING, 
					     (void*)gtk_entry_get_text(ent_r_invalid_chars));

	*rename_case_conv = gtk_combo_box_get_active(combo_r_case);
}


/* UI callbacks */
void cb_t_conv_space_none_toggled(GtkToggleButton *button, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(ent_t_conv_chars), !gtk_toggle_button_get_active(button));
}

void cb_r_conv_space_none_toggled(GtkToggleButton *button, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(ent_r_conv_chars), !gtk_toggle_button_get_active(button));
}

void cb_r_invalid_omit_toggled(GtkToggleButton *button, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(ent_r_invalid_chars), !gtk_toggle_button_get_active(button));
}

void cb_chconv_save(GtkButton *button, gpointer user_data)
{
	to_prefs();
}


/*** public functions *******************************************************/

chconv_tag_options chconv_get_tag_options()
{
	chconv_tag_options opt;

	opt.space_conv = (*tag_space_conv ? tag_space_conv_chars : NULL);
	opt.case_conv = *tag_case_conv;
		
	return opt;
}

chconv_rename_options chconv_get_rename_options()
{
	chconv_rename_options opt;

	opt.space_conv = (*rename_space_conv ? rename_space_conv_chars : NULL);
	opt.invalid_conv = (*rename_invalid_conv ? rename_invalid_conv_chars : NULL);
	opt.case_conv = *rename_case_conv;

	return opt;
}


void chconv_display(int tab)
{
	from_prefs();

	gtk_window_present(dlg_char_conv);
	gtk_notebook_set_page(nb_char_conv, tab);
}


void chconv_init(GladeXML *xml)
{
	int nocaseconv = 0;
	gboolean true = TRUE;
	gboolean false = FALSE;

	/* 
	 * get the widgets from glade
	 */

	dlg_char_conv = GTK_WINDOW(glade_xml_get_widget(xml, "dlg_char_conv"));
	nb_char_conv = GTK_NOTEBOOK(glade_xml_get_widget(xml, "nb_char_conv"));

	rb_t_conv_space_none = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_t_conv_space_none"));
	rb_t_conv_space_from = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_t_conv_space_from"));
	ent_t_conv_chars = GTK_ENTRY(glade_xml_get_widget(xml, "ent_t_conv_chars"));
	combo_t_case = GTK_COMBO_BOX(glade_xml_get_widget(xml, "combo_t_case"));

	rb_r_conv_space_none = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_r_conv_space_none"));
	rb_r_conv_space_to = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_r_conv_space_to"));
	rb_r_invalid_omit = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_r_invalid_omit"));
	rb_r_invalid_convert = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_r_invalid_convert"));
	ent_r_conv_chars = GTK_ENTRY(glade_xml_get_widget(xml, "ent_r_conv_chars"));
	ent_r_invalid_chars = GTK_ENTRY(glade_xml_get_widget(xml, "ent_r_invalid_chars"));
	combo_r_case = GTK_COMBO_BOX(glade_xml_get_widget(xml, "combo_r_case"));

	gtk_window_set_transient_for(dlg_char_conv, GTK_WINDOW(glade_xml_get_widget(xml, "w_main")));


	/*
	 * get the preference values, or set them to defaults
	 */

	tag_space_conv = pref_get_or_set("chconv:tag_space_conv", PREF_BOOLEAN, &false);
	tag_space_conv_chars = pref_get_or_set("chconv:tag_space_conv_chars", PREF_STRING, "_");
	tag_case_conv = pref_get_or_set("chconv:tag_case_conv", PREF_INT, &nocaseconv);

	rename_space_conv = pref_get_or_set("chconv:rename_space_conv", PREF_BOOLEAN, &false);
	rename_space_conv_chars = pref_get_or_set("chconv:rename_space_conv_chars", PREF_STRING, "_");
	rename_invalid_conv = pref_get_or_set("chconv:rename_invalid_conv", PREF_BOOLEAN, &true);
	rename_invalid_conv_chars = pref_get_or_set("chconv:rename_invalid_conv_chars", PREF_STRING, "-");
	rename_case_conv = pref_get_or_set("chconv:rename_case_conv", PREF_INT, &nocaseconv);


	/*
	 * synchronize the interface state
	 */
	from_prefs();
}


