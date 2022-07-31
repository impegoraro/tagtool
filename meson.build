project('tagtool', 'c',
	version: '0.14.5'
)

cc = meson.get_compiler('c')

tagtool_name    = meson.project_name()
tagtool_version = meson.project_version()

tagtool_prefix    = get_option('prefix')
prefix_datadir    = join_paths(tagtool_prefix, get_option('datadir'))
tagtool_bindir    = join_paths(tagtool_prefix, get_option('bindir'))
tagtool_datadir   = join_paths(tagtool_prefix, get_option('datadir'), 'tagtool')
tagtool_localedir = join_paths(tagtool_prefix, get_option('localedir'), 'tagtool')

config_h = configuration_data()

config_h.set_quoted('DATADIR', tagtool_datadir)
config_h.set_quoted('LOCALEDIR', tagtool_localedir)

set_defines = [
  ['PACKAGE', tagtool_name],
  ['PACKAGE_NAME', meson.project_name()],
  ['PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), tagtool_version)],
  ['PACKAGE_TARNAME', tagtool_name],
  ['PACKAGE_VERSION', tagtool_version],
  ['VERSION', tagtool_version],
  ['GETTEXT_PACKAGE', tagtool_name],
  ['PACKAGE_PREFIX', tagtool_prefix],
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach

subdir('src')
subdir('data')
subdir('pixmaps')

