dnl Process this file with autoconf to produce a configure script.


AC_INIT(tagtool, 0.12.3)
AM_INIT_AUTOMAKE(${PACKAGE_NAME}, ${PACKAGE_VERSION})

AC_CONFIG_SRCDIR([src/main.c])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

SPEC_RELEASE="1"
AC_SUBST(SPEC_RELEASE)


dnl Command line options
AC_ARG_ENABLE(mp3,
	AC_HELP_STRING([--disable-mp3], [Disable MP3 support (if enabled, id3lib is required)]),
	[enable_mp3=no;  disable_mp3_reason="(disabled)"],
	[enable_mp3=yes])

AC_ARG_ENABLE(vorbis,
	AC_HELP_STRING([--disable-vorbis], [Disable Ogg Vorbis support (if enabled, libvorbis is required)]),
	[enable_vorbis=no;  disable_vorbis_reason="(disabled)"],
	[enable_vorbis=yes])

if test "$enable_mp3" = "no" && test "$enable_vorbis" = "no"; then
	AC_MSG_ERROR([At least one of 'mp3' or 'vorbis' must be enabled. Try './configure --help' for a list of options.])
fi;


dnl Checks for programs.
AC_PROG_CC
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Checks for libraries and header files
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h langinfo.h],,
	[AC_MSG_ERROR([Missing necessary header files.])])

dnl Use gnuregex in FreeBSD
OSTYPE=`uname -s`
if test "$OSTYPE" = "FreeBSD"; then
	AC_CHECK_HEADERS([gnuregex.h gnu/regex.h])
	AC_CHECK_LIB(gnuregex, main,,
		[AC_MSG_ERROR([Missing required library])])
fi;


dnl Check for gtk and related libraries
PKG_CHECK_MODULES(GTK, [glib-2.0 >= 2.12.0
		       gtk+-2.0 >= 2.8.0
		       libglade-2.0 >= 2.6.0])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)


dnl Check for libid3 and libvorbis
if test "$enable_mp3" = "yes"; then
	AC_CHECK_HEADERS([id3.h],,
		[enable_mp3=no;  disable_mp3_reason="(missing id3lib headers)"])
fi;
if test "$enable_mp3" = "yes"; then
	dnl Fix missing link dependencies in libid3.so  (some distros have fixed 
	dnl this in their own libid3 packages, others haven't...)
	SAVE_LDFLAGS=$LDFLAGS
	LDFLAGS="-lstdc++ -lz $LDFLAGS"

	AC_CHECK_LIB(id3, main,,
		[enable_mp3=no;  disable_mp3_reason="(missing id3 library)"])
	AC_CHECK_LIB(id3, ID3FrameInfo_LongName,, 
		[AC_DEFINE(LIBID3_MISSING_ID3FRAMEINFO, 1, [Define to 1 if the ID3FrameInfo_* functions are missing from libid3])])

	if test "$enable_mp3" = "no"; then
		LDFLAGS=$SAVE_LDFLAGS
	fi;
fi;

if test "$enable_vorbis" = "yes"; then
	AC_CHECK_HEADERS([ogg/ogg.h vorbis/codec.h vorbis/vorbisfile.h],,
		[enable_vorbis=no;  disable_vorbis_reason="(missing vorbis headers)"])
fi;
if test "$enable_vorbis" = "yes"; then
	AC_CHECK_LIB(vorbis, main,,
		[enable_vorbis=no;  disable_vorbis_reason="(missing vorbis library)"])
fi;
if test "$enable_vorbis" = "yes"; then
	AC_CHECK_LIB(vorbisfile, main,,
		[enable_vorbis=no;  disable_vorbis_reason="(missing vorbisfile library)"])
fi;

AM_CONDITIONAL(ENABLE_MP3,    test "$enable_mp3" = "yes")
AM_CONDITIONAL(ENABLE_VORBIS, test "$enable_vorbis" = "yes")

if test "$enable_mp3" = "no" && test "$enable_vorbis" = "no"; then
	AC_MSG_RESULT([
No MP3 support $disable_mp3_reason
No Ogg Vorbis support $disable_vorbis_reason
])
	AC_MSG_ERROR([Neither MP3 or Vorbis support is being built. Cannot continue.])
fi;


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_STAT
AC_FUNC_FNMATCH
AC_FUNC_STRCOLL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([strcasecmp nl_langinfo regcomp],,
	[AC_MSG_ERROR([Missing necessary functions.])])


dnl Internationalization

AC_PROG_INTLTOOL([0.30])
GETTEXT_PACKAGE=tagtool
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
ALL_LINGUAS="pt fr cs es it en_GB de bg lt ua ru pl sv nl pt_BR"
AM_GLIB_GNU_GETTEXT
dnl AM_GNU_GETTEXT([external])


dnl Define useful directories

AM_GLIB_DEFINE_LOCALEDIR(LOCALEDIR)


dnl Output

AC_CONFIG_FILES([
	data/Makefile
	pixmaps/Makefile
	po/Makefile.in
	src/Makefile
	Makefile
	tagtool.spec
])
dnl	help/Makefile
dnl	help/C/Makefile
AC_OUTPUT


AC_MSG_RESULT([
Configuration information:

  MP3 support:         $enable_mp3  $disable_mp3_reason
  Ogg Vorbis support:  $enable_vorbis  $disable_vorbis_reason

Now run 'make' to compile $PACKAGE_NAME
])
