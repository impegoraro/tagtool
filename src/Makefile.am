## Process this file with automake to produce Makefile.in


EXTRA_DIST = tagtool.glade preferences.dtd

bin_PROGRAMS = tagtool


## resource files

resourcedir = $(pkgdatadir)
resource_DATA = tagtool.glade preferences.dtd


## tagtool binary

if ENABLE_MP3
mp3_sources = mpeg_file.c mpeg_file.h mpeg_edit.c mpeg_edit.h 		\
	mpeg_edit_field.c mpeg_edit_field.h aux_id3lib.c
mp3_cflags = -DENABLE_MP3
else
mp3_sources =
mp3_cflags =
endif


if ENABLE_VORBIS
vorbis_sources = vorbis_file.c vorbis_file.h vorbis_edit.c 		\
	vorbis_edit.h vorbis_edit_field.c vorbis_edit_field.h vcedit.c 	\
	vcedit.h
vorbis_cflags = -DENABLE_VORBIS
else
vorbis_sources =
vorbis_cflags =
endif


tagtool_CFLAGS = -Wall -Wno-pointer-sign $(GTK_CFLAGS) ${mp3_cflags} 	\
	${vorbis_cflags} -DDATADIR='"$(datadir)/${PACKAGE_NAME}"'

tagtool_LDFLAGS = -export-dynamic
tagtool_LDADD = $(GTK_LIBS)

tagtool_SOURCES = file_list.c file_list.h file_util.c file_util.h	\
	edit_tab.c edit_tab.h elist.c elist.h main.c math_util.c	\
	math_util.h genre.c genre.h mru.c mru.h prefs.c prefs.h		\
	prefs_versions.c prefs_versions.h rename_tab.c rename_tab.h	\
	playlist_tab.c playlist_tab.h tag_tab.c tag_tab.h main_win.c	\
	main_win.h progress_dlg.c progress_dlg.h help.c help.h		\
	audio_file.c audio_file.h char_conv_dlg.c char_conv_dlg.h 	\
	message_box.c message_box.h cursor.c cursor.h gtk_util.c 	\
	gtk_util.h glib_util.c glib_util.h str_util.c str_util.h 	\
	str_convert.c str_convert.h about.c about.h prefs_dlg.c 	\
	prefs_dlg.h rename_dlg.c rename_dlg.h status_bar.c status_bar.h	\
	clear_tab.c clear_tab.h scan_progress_dlg.c scan_progress_dlg.h	\
	chrono.c chrono.h tagtool.glade 				\
	${mp3_sources} ${vorbis_sources}

EXTRA_tagtool_SOURCES = mpeg_file.c mpeg_file.h mpeg_edit.c mpeg_edit.h	\
	mpeg_edit_field.c mpeg_edit_field.h aux_id3lib.c vorbis_file.c 	\
	vorbis_file.h vorbis_edit.c vorbis_edit.h vorbis_edit_field.c 	\
	vorbis_edit_field.h vcedit.c vcedit.h

