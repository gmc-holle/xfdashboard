themedir = $(datadir)/themes/xfdashboard-dark/xfdashboard-1.0

theme_in_files = \
	xfdashboard.theme.in

theme_DATA = \
	$(theme_in_files:.theme.in=.theme) \
	xfdashboard-dark.css \
	xfdashboard.css \
	xfdashboard.xml \
	xfdashboard-secondary.xml \
	animations.xml \
	effects.xml \
	screenshot-xfdashboard-dark.png

# Substitute bindir so it works when built in a non-system prefix
xfdashboard.theme.in:xfdashboard.theme.in.in
	$(AM_V_GEN) sed -e "s|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|" \
	    $< > $@ || rm $@

@INTLTOOL_THEME_RULE@

CLEANFILES = \
	$(theme_in_files) \
	$(theme_in_files:.theme.in=.theme)

EXTRA_DIST = \
	$(theme_in_files:.theme.in=.theme.in.in) \
	$(theme_DATA)

DISTCLEANFILES = \
	$(theme_in_files) \
	$(theme_in_files:.theme.in=.theme)
