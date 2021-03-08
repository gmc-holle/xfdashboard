AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFDASHBOARD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPACKAGE_LIBDIR=\"$(libdir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfdashboard

xfdashboard_headers = \
	application.h

xfdashboard_SOURCES = \
	$(xfdashboard_headers) \
	application.c \
	main.c

xfdashboard_CFLAGS = \
	$(CLUTTER_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfdashboard_LDADD = \
	$(CLUTTER_LIBS) \
	$(GLIB_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(GTK_LIBS) \
	$(top_builddir)/common/libxfconf-settings.la \
	$(top_builddir)/libxfdashboard/libxfdashboard.la

xfdashboard_LDFLAGS = \
	$(PLATFORM_LDFLAGS) \
	-export-dynamic \
	-no-undefined
