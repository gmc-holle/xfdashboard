SUBDIRS = \
	metainfo \
	data \
	icons \
	themes

desktopdir = $(datadir)/applications
desktop_in_files = org.xfce.xfdashboard.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = org.xfce.xfdashboard-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

settingsdir = $(datadir)/applications
settings_in_files = org.xfce.xfdashboard-settings.desktop.in
settings_DATA = $(settings_in_files:.desktop.in=.desktop)

# Substitute bindir so it works when built in a non-system prefix
org.xfce.xfdashboard.desktop.in:org.xfce.xfdashboard.desktop.in.in
	$(AM_V_GEN) sed -e "s|@bindir[@]|$(bindir)|" \
	    -e "s|@VERSION[@]|$(VERSION)|" \
	    $< > $@ || rm $@

org.xfce.xfdashboard-autostart.desktop.in:org.xfce.xfdashboard-autostart.desktop.in.in
	$(AM_V_GEN) sed -e "s|@bindir[@]|$(bindir)|" \
	    -e "s|@VERSION[@]|$(VERSION)|" \
	    $< > $@ || rm $@

org.xfce.xfdashboard-settings.desktop.in:org.xfce.xfdashboard-settings.desktop.in.in
	$(AM_V_GEN) sed -e "s|@bindir[@]|$(bindir)|" \
	    -e "s|@VERSION[@]|$(VERSION)|" \
	    $< > $@ || rm $@

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_files:.desktop.in=.desktop.in.in) \
	$(autostart_in_files:.desktop.in=.desktop.in.in) \
	$(settings_in_files:.desktop.in=.desktop.in.in)

CLEANFILES = \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(autostart_in_files) \
	$(autostart_DATA) \
	$(settings_in_files) \
	$(settings_DATA)
