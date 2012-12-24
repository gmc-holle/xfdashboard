INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DDATADIR=\"$(datadir)/xfce4\" \
	-DLIBDIR=\"$(libdir)/xfce4\" \
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
	application-icon.h \
	applications-view.h \
	button.h \
	common.h \
	drag-action.h \
	drop-action.h \
	drop-targets.h \
	fill-box-layout.h \
	live-window.h \
	quicklaunch.h \
	scaling-box-layout.h \
	scaling-flow-layout.h \
	scrollbar.h \
	searchbox.h \
	view.h \
	viewpad.h \
	view-selector.h \
	windows-view.h

xfdashboard_SOURCES = \
	$(xfdashboard_built_sources) \
	$(xfdashboard_headers) \
	application-icon.c \
	applications-view.c \
	button.c \
	common.c \
	drag-action.c \
	drop-action.c \
	drop-targets.c \
	fill-box-layout.c \
	live-window.c \
	main.c \
	quicklaunch.c \
	scaling-box-layout.c \
	scaling-flow-layout.c \
	scrollbar.c \
	searchbox.c \
	view.c \
	viewpad.c \
	view-selector.c \
	windows-view.c

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
	$(DBUS_GLIB_LIBS)

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
