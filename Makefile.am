if LIBWWW
DIR_GLIBWWW=glibwww
else
DIR_GLIBWWW=
endif

SUBDIRS = macros $(DIR_GLIBWWW) src


EXTRA_DIST = \
	html-component.gnorba

if BONOBO
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-component.gnorba
endif

