AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFDASHBOARD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPACKAGE_LIBDIR=\"$(libdir)\" \
	$(XFDASHBOARD_DEBUG_CFLAGS) \
	$(PLATFORM_CPPFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfdashboard.pc

lib_LTLIBRARIES = \
	libxfdashboard.la

xfdashboard_marshal_prefix = xfdashboard

libxfdashboard_la_built_sources = \
	enums.h \
	enums.c \
	marshal.h \
	marshal.c \
	version.h

libxfdashboard_la_headers = \
	action-button.h \
	actor.h \
	animation.h \
	application.h \
	application-button.h \
	application-database.h \
	application-tracker.h \
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
	compat.h \
	css-selector.h \
	debug.h \
	desktop-app-info.h \
	desktop-app-info-action.h \
	drag-action.h \
	drop-action.h \
	dynamic-table-layout.h \
	emblem-effect.h \
	fill-box-layout.h \
	focusable.h \
	focus-manager.h \
	image-content.h \
	label.h \
	live-window.h \
	live-window-simple.h \
	live-workspace.h \
	model.h \
	outline-effect.h \
	plugin.h \
	plugins-manager.h \
	popup-menu.h \
	popup-menu-item.h \
	popup-menu-item-button.h \
	popup-menu-item-separator.h \
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
	theme-animation.h \
	theme-css.h \
	theme-effects.h \
	theme-layout.h \
	toggle-button.h \
	tooltip-action.h \
	transition-group.h \
	types.h \
	utils.h \
	view.h \
	view-manager.h \
	view-selector.h \
	viewpad.h \
	window-content.h \
	window-tracker.h \
	window-tracker-backend.h \
	window-tracker-monitor.h \
	window-tracker-window.h \
	window-tracker-workspace.h \
	windows-view.h \
	workspace-selector.h

libxfdashboard_la_SOURCES = \
	$(libxfdashboard_la_built_sources) \
	$(xfdashboard_headers) \
	action-button.c \
	actor.c \
	animation.c \
	application.c \
	application-button.c \
	application-database.c \
	application-tracker.c \
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
	debug.c \
	desktop-app-info.c \
	desktop-app-info-action.c \
	drag-action.c \
	drop-action.c \
	dynamic-table-layout.c \
	emblem-effect.c \
	fill-box-layout.c \
	focusable.c \
	focus-manager.c \
	image-content.c \
	label.c \
	live-window.c \
	live-window-simple.c \
	live-workspace.c \
	model.c \
	outline-effect.c \
	plugin.c \
	plugins-manager.c \
	popup-menu.c \
	popup-menu-item.c \
	popup-menu-item-button.c \
	popup-menu-item-separator.c \
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
	theme-animation.c \
	theme-css.c \
	theme-effects.c \
	theme-layout.c \
	toggle-button.c \
	tooltip-action.c \
	transition-group.c \
	utils.c \
	view.c \
	view-manager.c \
	view-selector.c \
	viewpad.c \
	window-content.c \
	window-tracker.c \
	window-tracker-backend.c \
	window-tracker-monitor.c \
	window-tracker-window.c \
	window-tracker-workspace.c \
	windows-view.c \
	workspace-selector.c

libxfdashboard_la_CFLAGS = \
	$(LIBWNCK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(CLUTTER_COGL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(GARCON_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfdashboard_la_LIBADD = \
	$(LIBWNCK_LIBS) \
	$(CLUTTER_LIBS) \
	$(CLUTTER_COGL_LIBS) \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(GARCON_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBM)

if DBUS_GLIB
libxfdashboard_la_CFLAGS += \
	$(DBUS_GLIB_CFLAGS)

libxfdashboard_la_LIBADD += \
	$(DBUS_GLIB_LIBS)
endif

libxfdashboard_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBXFDASHBOARD_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined

x11_headers = \
	x11/window-content-x11.h \
	x11/window-tracker-x11.h \
	x11/window-tracker-backend-x11.h \
	x11/window-tracker-monitor-x11.h \
	x11/window-tracker-window-x11.h \
	x11/window-tracker-workspace-x11.h

x11_sources = \
	x11/window-content-x11.c \
	x11/window-tracker-x11.c \
	x11/window-tracker-backend-x11.c \
	x11/window-tracker-monitor-x11.c \
	x11/window-tracker-window-x11.c \
	x11/window-tracker-workspace-x11.c

libxfdashboard_la_headers += $(x11_headers)
libxfdashboard_la_SOURCES += $(x11_sources)

if XFDASHBOARD_BUILD_WITH_XCOMPOSITE
libxfdashboard_la_CFLAGS += \
	$(XCOMPOSITE_CFLAGS)

libxfdashboard_la_LIBADD += \
	$(XCOMPOSITE_LIBS)
endif

if XFDASHBOARD_BUILD_WITH_XDAMAGE
libxfdashboard_la_CFLAGS += \
	$(XDAMAGE_CFLAGS)

libxfdashboard_la_LIBADD += \
	$(XDAMAGE_LIBS)
endif

if XFDASHBOARD_BUILD_WITH_XINERAMA
libxfdashboard_la_CFLAGS += \
	$(XINERAMA_CFLAGS)

libxfdashboard_la_LIBADD += \
	$(XINERAMA_LIBS)
endif

if XFDASHBOARD_BUILD_WITH_BACKEND_GDK
gdk_headers = \
	gdk/window-tracker-backend-gdk.h

libxfdashboard_la_headers += $(gdk_headers)

gdk_sources = \
	gdk/window-tracker-backend-gdk.c

libxfdashboard_la_SOURCES += $(gdk_sources)

libxfdashboard_la_CFLAGS += \
	$(CLUTTER_GDK_CFLAGS)

libxfdashboard_la_LIBADD += \
	$(CLUTTER_GDK_LIBS)
endif

libxfdashboard_la_includedir = \
	$(includedir)/xfdashboard/libxfdashboard

libxfdashboard_la_include_HEADERS = \
	$(libxfdashboard_la_headers) \
	libxfdashboard.h

if MAINTAINER_MODE

CLEANFILES = \
	xgen-xmc \
	xgen-xmh

enums.h: $(libxfdashboard_la_headers) Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template enums.h.template $(libxfdashboard_la_headers) > $@

enums.c: $(libxfdashboard_la_headers) Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template enums.c.template $(libxfdashboard_la_headers) > $@

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

version.h: version.h.in Makefile
	$(AM_V_GEN) sed -e "s|@LIBXFDASHBOARD_MAJOR_VERSION[@]|$(XFDASHBOARD_VERSION_MAJOR)|" \
		-e "s|@LIBXFDASHBOARD_MINOR_VERSION[@]|$(XFDASHBOARD_VERSION_MINOR)|" \
		-e "s|@LIBXFDASHBOARD_MICRO_VERSION[@]|$(XFDASHBOARD_VERSION_MICRO)|" \
		$< > $@ || rm $@

DISTCLEANFILES = \
	$(libxfdashboard_la_built_sources)

BUILT_SOURCES = \
	$(libxfdashboard_la_built_sources)

endif

EXTRA_DIST = \
	enums.h.template \
	enums.c.template \
	marshal.list \
	version.h.in
