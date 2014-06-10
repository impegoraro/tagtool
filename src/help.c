#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "help.h"


/* help strings */

char *help_titles[] = {
/* HELP_TAG_FORMAT */
N_("Help - File Name Format"),

/* HELP_RENAME_FORMAT */
N_("Help - File Name Format")
};

char *help_strings[] = {
/* HELP_TAG_FORMAT */
N_("This field describes the expected format of the file name \n"
"(not including the extension).  Audio Tag Tool uses this \n"
"information to automatically fill in the tags based on the \n"
"file's name. \n"
"\n"
"Each tag field has a corresponding symbol, as listed below: \n"
"\n"
"  <title>\n"
"  <artist>\n"
"  <album>\n"
"  <year>\n"
"  <comment>\n"
"  <track>\n"
"  <genre>\n"
"\n"
"Examples: \n"
"\n"
"The format \"<artist> - <album> - <title>\" will match a \n"
"file named \"Pink Floyd - The Wall - Hey You.mp3\". In this \n"
"example the Artist, Album and Title fields of the tag will \n"
"be set to \"Pink Floyd\", \"The Wall\" and \"Hey You\", \n"
"respectively.\n"
"\n"
"The format \"<artist> - <*> - <title>\" could also be used \n"
"in the previous example if we did not want to fill in the \n"
"album name (the special symbol <*> causes a part of the \n"
"file name to be ignored.) \n"),

/* HELP_RENAME_FORMAT */
N_("This field describes the desired format of the file name. \n"
"Audio Tag Tool can use this information to rename files or \n"
"organize them into subdirectories, based on the content of \n"
"their tags. \n"
"\n"
"Each ID3 field has a corresponding symbol, as listed below: \n"
"\n"
"  <title>\n"
"  <artist>\n"
"  <album>\n"
"  <year>\n"
"  <comment>\n"
"  <track>\n"
"  <genre>\n"
"\n"
"Renaming example: \n"
"\n"
"To have all file names consist of the track number followed \n"
"by the track title, a format such as \"<track> - <title>\" can \n"
"be used. \n"
"\n"
"Moving example: \n"
"\n"
"The file name format can include sub-directories. If in the \n"
"previous example we had wanted the files to be placed in \n"
"a directory with the album name, we would have used the \n"
"format \"<album>/<track> - <title>\". \n")
};


/* widgets */
static GtkWindow *dlg_help = NULL;
static GtkLabel *lab_help = NULL;


/*** public functions *******************************************************/

void help_init(GladeXML *xml)
{
	dlg_help = GTK_WINDOW(glade_xml_get_widget(xml, "dlg_help"));
	lab_help = GTK_LABEL(glade_xml_get_widget(xml, "lab_help"));
}


void help_display(int topic)
{
	gtk_window_set_title(dlg_help, _(help_titles[topic]));
	gtk_label_set_text(lab_help, _(help_strings[topic]));

	gtk_widget_show(GTK_WIDGET(dlg_help));
}

