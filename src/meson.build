glib_dep         = dependency('glib-2.0')
gtk_dep          = dependency('gtk+-3.0')
xml_dep          = dependency('libxml-2.0')
vorbis_dep       = dependency('vorbis', required: false)
vorbisfiles_dep  = dependency('vorbisfile', required: false)
ogg_dep          = dependency('ogg', required: false)
m_dep            = cc.find_library('m')

## id3lib dependencies
#stdcpp_dep       = cc.find_library('stdc++')
#z_dep            = cc.find_library('z')
#id3_dep          = cc.find_library('libid3', required: false)

## checking for id3 header and lib /usr/include/id3.h
#if cc.has_header('id3.h')
  #config_h.set_quoted('ENABLE_MP3', '0')
#endif

if ogg_dep.found() and vorbis_dep.found() and vorbisfiles_dep.found()
  config_h.set_quoted('ENABLE_VORBIS', '1')
endif

sources = files(
  'vorbis_edit.c',
  'elist.c',
  'about.c',
  'str_convert.c',
  'str_util.c',
  'main_win.c',
  'vorbis_file.c',
  'vcedit.c',
  'clear_tab.c',
  'file_util.c',
  'status_bar.c',
  'rename_dlg.c',
  'cursor.c',
  'genre.c',
  'rename_tab.c',
  'vorbis_edit_field.c',
  'edit_tab.c',
  'main.c',
  'aux_id3lib.c',
  'scan_progress_dlg.c',
  'prefs_dlg.c',
  'prefs.c',
  'char_conv_dlg.c',
  'chrono.c',
  'audio_file.c',
  'mru.c',
  'prefs_versions.c',
  #'mpeg_edit_field.c',
  #'mpeg_edit.c',
  #'mpeg_file.c',
  'playlist_tab.c',
  'tag_tab.c',
  'gtk_util.c',
  'file_list.c',
  'progress_dlg.c',
  'math_util.c',
  'glib_util.c',
  'help.c',
  'message_box.c'
)

configure_file(
  output: 'config.h',
  configuration: config_h
)
configuration_inc = include_directories('.')

data_files = files(
  'tagtool.glade',
  'basic.css',
  'preferences.dtd'
)
install_data(data_files, install_dir: tagtool_datadir)

executable('tagtool', sources, dependencies: [
	glib_dep,
	gtk_dep,
	xml_dep,
	m_dep,
	#stdcpp_dep,
	#z_dep,
	#id3_dep,
	ogg_dep,
	vorbis_dep,
	vorbisfiles_dep],
	include_directories: configuration_inc,
	install: true,
	link_args : '-rdynamic'
)

