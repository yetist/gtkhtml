if LIBWWW
DIR_GLIBWWW=glibwww
else
DIR_GLIBWWW=
endif

SUBDIRS = macros src $(DIR_GLIBWWW)


EXTRA_DIST = \
	html-component.gnorba

if BONOBO
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-component.gnorba
endif

