if LIBWWW
DIR_GLIBWWW=glibwww
else
DIR_GLIBWWW=
endif

confexecdir = $(libdir)
confexec_DATA = gtkhtmlConf.sh

EXTRA_DIST = gtkhtmlConf.sh.in

gtkhtmlConf.sh: gtkhtmlConf.sh.in Makefile
	sed -e 's?\@GTKHTML_LIBDIR\@?$(GTKHTML_LIBDIR)?g' \
	    -e 's?\@GTKHTML_INCLUDEDIR\@?$(GTKHTML_INCLUDEDIR)?g' \
	    -e 's?\@VERSION\@?$(VERSION)?g' \
	    -e 's?\@GTKHTML_LIBS\@?$(GTKHTML_LIBS)?g' \
	      < $(srcdir)/gtkhtmlConf.sh.in > gtkhtmlConf.tmp \
	  && mv gtkhtmlConf.tmp gtkhtmlConf.sh

SUBDIRS = macros $(DIR_GLIBWWW) src


EXTRA_DIST = \
	html-component.gnorba

if BONOBO
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-component.gnorba
endif

