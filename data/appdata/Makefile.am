appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = xfdashboard.appdata.xml.in

@INTLTOOL_XML_RULE@

DISTCLEANFILES =	\
	$(appdata_DATA)
