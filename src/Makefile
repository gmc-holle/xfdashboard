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
	livewindow.c \
	main.c \
	quicklaunch.c \
	quicklaunch-icon.c \
	scalingflowlayout.c \
	view.c \
	windows-view.c

HEADERS = \
	livewindow.h \
	main.h \
	quicklaunch.h \
	quicklaunch-icon.h \
	scalingflowlayout.h \
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
