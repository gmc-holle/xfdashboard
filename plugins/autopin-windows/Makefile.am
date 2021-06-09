plugindir = $(libdir)/xfdashboard/plugins
PLUGIN_ID = autopin-windows

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard-plugin-autopin-windows\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"$(PLUGIN_ID)\"

plugin_LTLIBRARIES = \
	autopin-windows.la

autopin_windows_la_SOURCES = \
	autopin-windows.c \
	autopin-windows.h \
	plugin.c

autopin_windows_la_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(GARCON_CFLAGS) \
	$(PLATFORM_CFLAGS)

autopin_windows_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-export-symbols-regex '^plugin_init$$' \
	-no-undefined \
	-module \
	-shared \
	$(PLATFORM_LDFLAGS)

autopin_windows_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(GARCON_CLIBS) \
	$(top_builddir)/libxfdashboard/libxfdashboard.la

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
