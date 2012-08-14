# A simple Makefile unless I can create a real autotools-based one ;)

PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

PKG_LIBWNCK="libwnck-1.0"
PKG_CLUTTER="clutter-1.0 clutter-cogl-1.0"

# Uncomment if you going to use libwnck prior to version 3.0
#LIBWNCK_VERSION=`pkg-config --modversion ${PKG_LIBWNCK} | sed -re 's/([0-9]+)\.([0-9]+)\.([0-9]+)/-DWNCK_MAJOR_VERSION=\1 -DWNCK_MINOR_VERSION=\2 -DWNCK_MICRO_VERSION=\3/'`

PKGS="${PKG_LIBWNCK} ${PKG_CLUTTER}"

CFLAGS += `pkg-config ${PKGS} --cflags` -pedantic -Wall ${LIBWNCK_VERSION}
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
