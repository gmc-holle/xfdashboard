AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFDASHBOARD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"xfdashboard\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(XFDASHBOARD_DEBUG_CFLAGS) \
	$(PLATFORM_CPPFLAGS)

noinst_LTLIBRARIES = \
	libxfconf-settings.la

libxfconf_settings_la_SOURCES = \
	xfconf-settings.c \
	xfconf-settings.h

libxfconf_settings_la_CFLAGS = \
	$(CLUTTER_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(XFDASHBOARD_DEBUG_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfconf_settings_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-no-undefined \
	-module \
	-shared \
	$(PLATFORM_LDFLAGS)

libxfconf_settings_la_LIBADD = \
	$(CLUTTER_LIBS) \
	$(LIBXFCONF_LIBS)

CLEANFILES = \
	$(noinst_DATA)

EXTRA_DIST = \
	$(noinst_DATA)

DISTCLEANFILES = \
	$(noinst_DATA)
