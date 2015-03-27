AM_CPPFLAGS = \
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
	application-database.h \
	applications-menu-model.h \
	applications-search-provider.h \
	applications-view.h \
	background.h \
	binding.h \
	bindings-pool.h \
	box-layout.h \
	button.h \
	click-action.h \
	collapse-box.h \
	css-selector.h \
	desktop-app-info.h \
	drag-action.h \
	drop-action.h \
	dynamic-table-layout.h \
	fill-box-layout.h \
	focusable.h \
	focus-manager.h \
	image-content.h \
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
	stage-interface.h \
	stylable.h \
	text-box.h \
	theme.h \
	theme-css.h \
	theme-effects.h \
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
	window-tracker-monitor.h \
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
	application-database.c \
	applications-menu-model.c \
	applications-search-provider.c \
	applications-view.c \
	background.c \
	binding.c \
	bindings-pool.c \
	box-layout.c \
	button.c \
	click-action.c \
	collapse-box.c \
	css-selector.c \
	desktop-app-info.c \
	drag-action.c \
	drop-action.c \
	dynamic-table-layout.c \
	fill-box-layout.c \
	focusable.c \
	focus-manager.c \
	image-content.c \
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
	stage-interface.c \
	stylable.c \
	text-box.c \
	theme.c \
	theme-css.c \
	theme-effects.c \
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
	window-tracker-monitor.c \
	window-tracker-window.c \
	window-tracker-workspace.c \
	windows-view.c \
	workspace-selector.c

xfdashboard_CFLAGS = \
	$(LIBWNCK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(CLUTTER_COGL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(GARCON_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
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
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(GARCON_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(DBUS_GLIB_LIBS) \
	-lm

if XFDASHBOARD_BUILD_WITH_XCOMPOSITE
xfdashboard_CFLAGS += \
	$(XCOMPOSITE_CFLAGS)

xfdashboard_LDADD += \
	$(XCOMPOSITE_LIBS)
endif

if XFDASHBOARD_BUILD_WITH_XDAMAGE
xfdashboard_CFLAGS += \
	$(XDAMAGE_CFLAGS)

xfdashboard_LDADD += \
	$(XDAMAGE_LIBS)
endif

if XFDASHBOARD_BUILD_WITH_XINERAMA
xfdashboard_CFLAGS += \
	$(XINERAMA_CFLAGS)

xfdashboard_LDADD += \
	$(XINERAMA_LIBS)
endif

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
