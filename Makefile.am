if BONOBO
DIR_COMPONENTS = components
else
DIR_COMPONENTS =
endif

confexecdir = $(libdir)
confexec_DATA = gtkhtmlConf.sh

gtkhtmlConf.sh: gtkhtmlConf.sh.in Makefile
	sed -e 's?\@GTKHTML_LIBDIR\@?$(GTKHTML_LIBDIR)?g' \
	    -e 's?\@GTKHTML_INCLUDEDIR\@?$(GTKHTML_INCLUDEDIR)?g' \
	    -e 's?\@VERSION\@?$(VERSION)?g' \
	    -e 's?\@GTKHTML_LIBS\@?$(GTKHTML_LIBS)?g' \
	      < $(srcdir)/gtkhtmlConf.sh.in > gtkhtmlConf.tmp \
	  && mv gtkhtmlConf.tmp gtkhtmlConf.sh

SUBDIRS =					\
	macros					\
	src					\
	capplet					\
	po					\
	doc					\
	$(DIR_COMPONENTS)

EXTRA_DIST =					\
	html-component.gnorba			\
	gtkhtml.spec.in				\
	gtkhtml.spec                            \
	COPYING.LIB				\
	BUGS					\
	gtkhtmlConf.sh.in

if BONOBO
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-component.gnorba
endif
