xfdashboard
===========

Maybe a Gnome shell like dashboard for Xfce


Requirements
============

libwnck >= 2.30
clutter >= 1.12
glib >= 2.32
gio >= 2.32
gio-unix-2.0 >= 2.32
xfconf >= 4.10.0
garcon >= 0.2.0
gtk+ >= 3.2
libxfce4util >= 4.10.0
libxfce4ui >= 4.10.0

... and the dependencies of these libraries of course ;)


The following libraries are semi-optional:

dbus-glib >= 0.98 (required if xfconf < 4.13)


The following libraries are optional and configurable:

libXcomposite >= 0.2 (for live windows)
libXdamage (for live windows)
libXinerama (for multi-monitor support)


On debian based distros, all requirements are installed with:

> apt-get install xfce4-dev-tools build-essential glib2.0 libglib2.0-dev xorg-dev libwnck-3-dev libclutter-1.0-dev libgarcon-1-0-dev libxfconf-0-dev libxfce4util-dev libxfce4ui-2-dev libxcomposite-dev libxdamage-dev libxinerama-dev


Homepage
========

The homepage of xfdashboard is at https://docs.xfce.org/apps/xfdashboard/start


Documentation
=============

A simple quick guide with screenshots can be found at http://docs.xfce.org/apps/xfdashboard/manual

Documentation about theming xfdashboard can be found at http://xfdashboard.froevel.de/theming.html

Documentation about settings in xfdashboard can be found at http://xfdashboard.froevel.de/settings.html

How to report bugs?
===================

You can report bugs and feature requests at http://bugzilla.xfce.org.
Choose the product Apps and the component xfdashboard.
You will need to create an account for yourself.

You can also join the Xfce development mailing-list:
https://mail.xfce.org/mailman/listinfo/xfce4-dev
