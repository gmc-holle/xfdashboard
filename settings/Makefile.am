AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPACKAGE_LIBDIR=\"$(libdir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfdashboard-settings

xfdashboard_settings_headers = \
	general.h \
	settings.h \
	themes.h

xfdashboard_settings_SOURCES = \
	$(xfdashboard_settings_headers) \
	general.c \
	main.c \
	settings.c \
	themes.c

xfdashboard_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfdashboard_settings_LDADD = \
	$(GTK_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBM) \
	$(top_builddir)/libxfdashboard/libxfdashboard.la

xfdashboard_settings_LDFLAGS = \
	-export-dynamic \
	-no-undefined \
	$(PLATFORM_LDFLAGS)
