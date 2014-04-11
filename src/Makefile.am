INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfdashboard

xfdashboard_marshal_prefix = xfdashboard

xfdashboard_built_sources = \
	enums.h \
	enums.c \
	marshal.h \
	marshal.c

xfdashboard_headers = \
	actor.h \
	application.h \
	application-button.h \
	applications-menu-model.h \
	applications-search-provider.h \
	applications-view.h \
	background.h \
	button.h \
	click-action.h \
	collapse-box.h \
	drag-action.h \
	drop-action.h \
	fill-box-layout.h \
	focusable.h \
	focus-manager.h \
	image.h \
	layoutable.h \
	live-window.h \
	live-workspace.h \
	outline-effect.h \
	quicklaunch.h \
	scaled-table-layout.h \
	scrollbar.h \
	search-manager.h \
	search-provider.h \
	search-result-container.h \
	search-result-set.h \
	search-view.h \
	stage.h \
	stylable.h \
	text-box.h \
	theme.h \
	theme-css.h \
	theme-layout.h \
	toggle-button.h \
	tooltip-action.h \
	types.h \
	utils.h \
	view.h \
	view-manager.h \
	view-selector.h \
	viewpad.h \
	window-content.h \
	window-tracker.h \
	window-tracker-window.h \
	window-tracker-workspace.h \
	windows-view.h \
	workspace-selector.h

xfdashboard_SOURCES = \
	$(xfdashboard_built_sources) \
	$(xfdashboard_headers) \
	actor.c \
	application.c \
	application-button.c \
	applications-menu-model.c \
	applications-search-provider.c \
	applications-view.c \
	background.c \
	button.c \
	click-action.c \
	collapse-box.c \
	drag-action.c \
	drop-action.c \
	fill-box-layout.c \
	focusable.c \
	focus-manager.c \
	image.c \
	layoutable.c \
	live-window.c \
	live-workspace.c \
	main.c \
	outline-effect.c \
	quicklaunch.c \
	scaled-table-layout.c \
	scrollbar.c \
	search-manager.c \
	search-provider.c \
	search-result-container.c \
	search-result-set.c \
	search-view.c \
	stage.c \
	stylable.c \
	text-box.c \
	theme.c \
	theme-css.c \
	theme-layout.c \
	toggle-button.c \
	tooltip-action.c \
	utils.c \
	view.c \
	view-manager.c \
	view-selector.c \
	viewpad.c \
	window-content.c \
	window-tracker.c \
	window-tracker-window.c \
	window-tracker-workspace.c \
	windows-view.c \
	workspace-selector.c

xfdashboard_CFLAGS = \
	$(LIBWNCK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(CLUTTER_COGL_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(GARCON_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfdashboard_LDFLAGS = \
	-export-dynamic \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

xfdashboard_LDADD = \
	$(LIBWNCK_LIBS) \
	$(CLUTTER_LIBS) \
	$(CLUTTER_COGL_LIBS) \
	$(GIO_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(GARCON_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(DBUS_GLIB_LIBS) \
	-lm

if MAINTAINER_MODE

CLEANFILES = \
	xgen-xmc \
	xgen-xmh

enums.h: $(xfdashboard_headers) Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template enums.h.template $(xfdashboard_headers) > $@

enums.c: $(xfdashboard_headers) Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template enums.c.template $(xfdashboard_headers) > $@

marshal.h: stamp-marshal.h
	@true

stamp-marshal.h: marshal.list Makefile
	$(AM_V_GEN) \
	( \
		cd $(srcdir) \
		&& ( $(GLIB_GENMARSHAL) \
			--prefix=_xfdashboard_marshal \
			--header marshal.list ) > xgen-xmh \
		&& ( cmp -s xgen-xmh marshal.h || cp xgen-xmh marshal.h ) \
		&& rm -f xgen-xmh \
		&& echo timestamp > $(@F) \
	)

marshal.c: marshal.list Makefile
	$(AM_V_GEN) \
	( \
		cd $(srcdir) \
		&& echo "#include \"marshal.h\"" > xgen-xmc \
		&& ( $(GLIB_GENMARSHAL) \
			--prefix=_xfdashboard_marshal \
			--body marshal.list ) >> xgen-xmc \
		&& cp xgen-xmc marshal.c \
		&& rm -f xgen-xmc \
	)

DISTCLEANFILES = \
	$(xfdashboard_built_sources)

BUILT_SOURCES = \
	$(xfdashboard_built_sources)

endif

EXTRA_DIST = \
	enums.h.template \
	enums.c.template \
	marshal.list
