# A simple Makefile unless I can create a real autotools-based one ;)

PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

PKG_LIBWNCK="libwnck-1.0"
PKG_CLUTTER="clutter-1.0 clutter-cogl-1.0"
PKG_GIO="gio-2.0 gio-unix-2.0"

PKGS="${PKG_LIBWNCK} ${PKG_CLUTTER} ${PKG_GIO}"

CFLAGS += -std=gnu99 `pkg-config ${PKGS} --cflags` -pedantic -Wall -g
LDFLAGS += `pkg-config ${PKGS} --libs`

SOURCES = \
	application-icon.c \
	live-window.c \
	main.c \
	quicklaunch.c \
	scaling-box-layout.c \
	scaling-flow-layout.c \
	view.c \
	windows-view.c

HEADERS = \
	application-icon.h \
	live-window.h \
	main.h \
	quicklaunch.h \
	scaling-box-layout.h \
	scaling-flow-layout.h \
	view.h \
	windows-view.h

all: xfdashboard

xfdashboard: Makefile ${SOURCES} ${HEADERS}
	gcc ${CFLAGS} -o xfdashboard ${SOURCES} ${LDFLAGS}

clean:
	rm -f xfdashboard *.o

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 xfdashboard ${DESTDIR}${BINDIR}/xfdashboard
