#include <gtk/gtk.h>
#include <glade/glade.h>

#include "gtk_util.h"
#include "prefs.h"

#include "prefs_dlg.h"


/* options */
static int *write_id3_version;
static gboolean *preserve_existing;


/* widgets */
static GtkWindow *dlg_prefs = NULL;
static GtkRadioButton *rb_ver_v1 = NULL;
static GtkRadioButton *rb_ver_v2 = NULL;
static GtkRadioButton *rb_ver_both = NULL;
static GtkRadioButton *rb_ver_auto = NULL;
static GtkCheckButton *cb_ver_preserve = NULL;


/*** private functions ******************************************************/

/* sets the interface state according to the preferences */
static void from_prefs()
{
	if (*write_id3_version == WRITE_V1)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_ver_v1), TRUE);
	else if (*write_id3_version == WRITE_V2)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_ver_v2), TRUE);
	else if (*write_id3_version == (WRITE_V1 | WRITE_V2))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_ver_both), TRUE);
	else if (*write_id3_version == WRITE_AUTO)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_ver_auto), TRUE);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_ver_preserve), *preserve_existing);
}

/* sets the preferences according to the curent interface state */
static void to_prefs()
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_ver_v1)))
		*write_id3_version = WRITE_V1;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_ver_v2)))
		*write_id3_version = WRITE_V2;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_ver_both)))
		*write_id3_version = WRITE_V1 | WRITE_V2;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_ver_auto)))
		*write_id3_version = WRITE_AUTO;

	*preserve_existing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_ver_preserve));
}


/* UI callbacks */
void cb_prefs_save(GtkButton *button, gpointer user_data)
{
	to_prefs();
}


/*** public functions *******************************************************/

id3_prefs prefs_get_id3_prefs()
{
	id3_prefs prefs;

	prefs.write_id3_version = *write_id3_version;
	prefs.preserve_existing = *preserve_existing;

	return prefs;
}


void prefs_display()
{
	from_prefs();

	gtk_window_present(dlg_prefs);
}


void prefs_init(GladeXML *xml)
{
	gboolean true = TRUE;
	int write_auto = WRITE_AUTO;

	/* 
	 * get the widgets from glade
	 */

	dlg_prefs = GTK_WINDOW(glade_xml_get_widget(xml, "dlg_prefs"));
	rb_ver_v1 = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_ver_v1"));
	rb_ver_v2 = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_ver_v2"));
	rb_ver_both = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_ver_both"));
	rb_ver_auto = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "rb_ver_auto"));
	cb_ver_preserve = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "cb_ver_preserve"));

	gtk_window_set_transient_for(dlg_prefs, GTK_WINDOW(glade_xml_get_widget(xml, "w_main")));


	/*
	 * get the preference values, or set them to defaults
	 */

	write_id3_version = pref_get_or_set("pref:write_id3_version", PREF_INT, &write_auto);
	preserve_existing = pref_get_or_set("pref:preserve_existing", PREF_BOOLEAN, &true);

	/*
	 * synchronize the interface state
	 */
	from_prefs();
}


