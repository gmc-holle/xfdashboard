# A simple Makefile unless I can create a real autotools-based one ;)

PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

PKG_LIBWNCK="libwnck-1.0"
PKG_CLUTTER="clutter-1.0 clutter-cogl-1.0"

PKGS="${PKG_LIBWNCK} ${PKG_CLUTTER}"

CFLAGS += -std=gnu99 `pkg-config ${PKGS} --cflags` -pedantic -Wall
LDFLAGS += `pkg-config ${PKGS} --libs`

SOURCES = main.c winlist.c
HEADERS = main.h winlist.h

all: xfdashboard

xfdashboard: Makefile ${SOURCES} ${HEADERS}
	g++ ${CFLAGS} -o xfdashboard ${SOURCES} ${LDFLAGS}

clean:
	rm -f xfdashboard *.o

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 xfdashboard ${DESTDIR}${BINDIR}/xfdashboard
