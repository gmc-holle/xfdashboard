plugindir = $(libdir)/xfdashboard/plugins

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfdashboard-plugin-gnome_shell_search_provider\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"de.froevel.xfdashboard.gnome-shell-search-provider\" \
	-DGNOME_SHELL_PROVIDERS_PATH=\"$(prefix)/share/gnome-shell/search-providers\"

plugin_LTLIBRARIES = \
	gnome-shell-search-provider.la

gnome_shell_search_provider_la_SOURCES = \
	gnome-shell-search-provider.c \
	gnome-shell-search-provider.h \
	plugin.c

gnome_shell_search_provider_la_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(GARCON_CFLAGS) \
	$(PLATFORM_CFLAGS)

gnome_shell_search_provider_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-no-undefined \
	-module \
	$(PLATFORM_LDFLAGS)

gnome_shell_search_provider_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(GARCON_LIBS) \
	$(top_builddir)/libxfdashboard/libxfdashboard.la

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
