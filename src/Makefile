# A simple Makefile unless I can create a real autotools-based one ;)

PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

PKG_LIBWNCK="libwnck-1.0"
PKG_CLUTTER="clutter-1.0 clutter-cogl-1.0"
PKG_GIO="gio-2.0 gio-unix-2.0"
PKG_GARCON="garcon-1"
PKG_XFCONF="libxfconf-0 dbus-glib-1"

PKGS="${PKG_LIBWNCK} ${PKG_CLUTTER} ${PKG_GIO} ${PKG_GARCON} ${PKG_XFCONF}"

CFLAGS += -std=gnu99 `pkg-config ${PKGS} --cflags` -pedantic -Wall -g
LDFLAGS += `pkg-config ${PKGS} --libs`

GLIB_MKENUMS=/usr/bin/glib-mkenums

SOURCES = \
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

HEADERS = \
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

ENUMS_SOURCE = \
	enums.c

ENUMS_HEADER = \
	enums.h
	
all: xfdashboard

enums.h: enums.h.template ${HEADERS}
	$(GLIB_MKENUMS) --template enums.h.template ${HEADERS} > $@

enums.c: enums.c.template ${SOURCES} ${HEADERS}
	$(GLIB_MKENUMS) --template enums.c.template ${HEADERS} > $@

xfdashboard: Makefile ${ENUMS_HEADER} ${ENUMS_SOURCE} ${SOURCES} ${HEADERS}
	gcc ${CFLAGS} -o xfdashboard ${ENUMS_SOURCE} ${SOURCES} ${LDFLAGS}

clean:
	rm -f xfdashboard *.o

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 xfdashboard ${DESTDIR}${BINDIR}/xfdashboard
