plugindir = $(libdir)/xfdashboard/plugins

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard-plugin-hot_corner\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"de.froevel.xfdashboard.hot-corner\"

plugin_LTLIBRARIES = \
	hot-corner.la

hot_corner_la_SOURCES = \
	hot-corner.c \
	hot-corner.h \
	plugin.c

hot_corner_la_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(PLATFORM_CFLAGS)

hot_corner_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-no-undefined \
	-module \
	$(PLATFORM_LDFLAGS)

hot_corner_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(top_builddir)/libxfdashboard/libxfdashboard.la

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
