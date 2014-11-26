themedir = $(datadir)/themes/xfdashboard-mint/xfdashboard-1.0

theme_in_files = \
	xfdashboard.theme.in

theme_DATA = \
	$(theme_in_files:.theme.in=.theme) \
	xfdashboard-mint.css \	
	xfdashboard.css \
	xfdashboard.xml \
	effects.xml

@INTLTOOL_THEME_RULE@

EXTRA_DIST = \
	$(theme_DATA) \
	xfdashboard.theme.in

DISTCLEANFILES = \
	$(theme_in_files:.theme.in=.theme)
