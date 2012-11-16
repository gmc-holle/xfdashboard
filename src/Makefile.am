INCLUDES = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DDATADIR=\"$(datadir)/xfce4\" \
	-DLIBDIR=\"$(libdir)/xfce4\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfdashboard

xfdashboard_built_sources = \
	enums.h \
	enums.c

xfdashboard_headers = \
	application-icon.h \
	applications-view.h \
	button.h \
	common.h \
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

enums.h: $(xfdashboard_headers) Makefile
	$(AM_V_GEN) glib-mkenums --template enums.h.template $(xfdashboard_headers) > $@

enums.c: $(xfdashboard_headers) Makefile
	$(AM_V_GEN) glib-mkenums --template enums.c.template $(xfdashboard_headers) > $@

DISTCLEANFILES = \
	$(xfdashboard_built_sources)

BUILT_SOURCES = \
	$(xfdashboard_built_sources)

endif

EXTRA_DIST = \
	enums.h.template \
	enums.c.template

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
