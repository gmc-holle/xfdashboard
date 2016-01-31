plugindir = $(libdir)/xfdashboard/plugins

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/xfdashboard \
	-DG_LOG_DOMAIN=\"xfdashboard-plugin-gnome_shell_search_provider\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DGNOME_SHELL_PROVIDERS_PATH=\"$(prefix)/share/gnome-shell/search-providers\"

plugin_LTLIBRARIES = \
	gnome-shell-search-provider.la

gnome_shell_search_provider_la_SOURCES = \
	gnome-shell-search-provider.c \
	gnome-shell-search-provider.h \
	plugin.c

gnome_shell_search_provider_la_CFLAGS = \
	$(PLATFORM_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(GARCON_CFLAGS)

gnome_shell_search_provider_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-module \
	$(PLATFORM_LDFLAGS)

gnome_shell_search_provider_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(GARCON_LIBS)

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
