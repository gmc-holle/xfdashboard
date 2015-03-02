AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfdashboard-settings

xfdashboard_settings_headers = \
	settings.h

xfdashboard_settings_SOURCES = \
	$(xfdashboard_settings_headers) \
	main.c \
	settings.c

xfdashboard_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfdashboard_settings_LDFLAGS = \
	-export-dynamic \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

xfdashboard_settings_LDADD = \
	$(GTK_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBM)
