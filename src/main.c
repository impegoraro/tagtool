#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glade/glade.h>

#include "elist.h"
#include "mru.h"
#include "prefs.h"
#include "main_win.h"
#include "file_list.h"
#include "status_bar.h"
#include "edit_tab.h"
#include "tag_tab.h"
#include "clear_tab.h"
#include "rename_tab.h"
#include "playlist_tab.h"
#include "rename_dlg.h"
#include "prefs_dlg.h"
#include "char_conv_dlg.h"
#include "progress_dlg.h"
#include "scan_progress_dlg.h"
#include "cursor.h"
#include "help.h"
#include "about.h"
#include "audio_file.h"
#ifdef ENABLE_MP3
#  include "mpeg_edit.h"
#  include "mpeg_edit_field.h"
#endif
#ifdef ENABLE_VORBIS
#  include "vorbis_edit.h"
#  include "vorbis_edit_field.h"
#endif


int print_version()
{
	printf("Audio Tag Tool %s\n", PACKAGE_VERSION);

	printf(_("Supported audio formats:  "));
#ifdef ENABLE_MP3
	printf("MPEG  ");
#endif
#ifdef ENABLE_VORBIS
	printf("Ogg Vorbis  ");
#endif
	printf("\n\n");

	printf("Copyright (c) 2004 Pedro Lopes\n"
	       "http://pwp.netcabo.pt/paol/tagtool/\n\n");

	return 0;
}


int print_help()
{
	printf( _("Usage:\n"
		  "  tagtool [DIR]\t\tStart in directory DIR\n"
		  "  tagtool --dump FILE\tDump all known information about FILE and exit\n"
		  "  tagtool --help\tPrint this help message\n"
		  "  tagtool --version\tPrint program version\n\n") );
	return 0;
}


int print_dump(char *filename)
{
	audio_file *af;
	int res;

	res = audio_file_new(&af, filename, FALSE);
	if (res != AF_OK) {
		if (res == AF_ERR_FILE)
			printf(_("Couldn't open file: %s\n\n"), filename);
		else if (res == AF_ERR_FORMAT)
			printf(_("File audio format not recognized: %s\n\n"), filename);
		return 1;
	}
	audio_file_dump(af);
	audio_file_delete(af);
	return 0;
}


void write_preferences()
{
	pref_write_file();
}


int main(int argc, char *argv[])
{
	char *start_dir = NULL;
	GladeXML *xml;
	gboolean res;
	int i;

	/* gettext initialization */
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	/* options that can be processed before gtk_init() */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--dump") == 0) {
			if (argc < i+2)
				return print_help();
			else
				return print_dump(argv[i+1]);
		}
		else if (strcmp(argv[i], "--help") == 0) {
			return print_help();
		}
		else if (strcmp(argv[i], "--version") == 0) {
			return print_version();
		}
	}

	gtk_init(&argc, &argv);

	if (argc > 1)
		start_dir = argv[1];

	/* load the interface */
	xml = glade_xml_new(DATADIR"/tagtool.glade", NULL, NULL);
	if (xml == NULL)
		g_error("Could not load the interface!");

	/* loading and saving preferences */
	res = pref_read_file();
	if (!res)
		g_warning("Failed to read preferences file");
	atexit(write_preferences);

	/* Set default icon & application name */
	g_set_application_name("Audio Tag Tool");
	gtk_window_set_default_icon_name("TagTool");

	/* let other modules get what they need from the GladeXML object */
	mw_init(xml);
	fl_init(xml);
	sb_init(xml);
	et_init(xml);
	tt_init(xml);
	ct_init(xml);
	rt_init(xml);
	pt_init(xml);
	pd_init(xml);
	spd_init(xml);
	rename_init(xml);
	prefs_init(xml);
	chconv_init(xml);
	cursor_init(xml);
	help_init(xml);
	about_init(xml);
#ifdef ENABLE_MP3
	mpeg_edit_init(xml);
	mpeg_editfld_init(xml);
#endif
#ifdef ENABLE_VORBIS
	vorbis_edit_init(xml);
	vorbis_editfld_init(xml);
#endif

	/* load the initial directory */
	fl_set_initial_dir(start_dir);

	/* connect the signals */
	glade_xml_signal_autoconnect(xml);

	/* start the GTK event loop */
	gtk_main();

	return 0;
}
