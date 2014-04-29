SUBDIRS =								\
	themes

desktopdir = $(datadir)/applications
desktop_in_files = xfdashboard.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = xfdashboard-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

# Substitute bindir so it works when built in a non-system prefix
%.desktop.in:%.desktop.in.in
	$(AM_V_GEN) sed -e "s|@bindir[@]|$(bindir)|" \
	    -e "s|@VERSION[@]|$(VERSION)|" \
	    $< > $@ || rm $@

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_files) \
	$(autostart_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) \
	$(autostart_DATA)
