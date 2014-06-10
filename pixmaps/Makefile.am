
icon1dir = $(datadir)/icons/hicolor/48x48/apps
icon1_DATA = TagTool.png

icon2dir = $(datadir)/icons/hicolor/scalable/apps
icon2_DATA = TagTool.svg

resourcedir = $(pkgdatadir)
resource_DATA = TagTool.png fail.png graydot.png greendot.png info.png 	\
	ok.png warn.png edit.png file.png folder.png tab_edit.png 	\
	tab_tag.png tab_clear.png tab_rename.png tab_playlist.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = Makefile.am $(resource_DATA) $(icon2_DATA)
