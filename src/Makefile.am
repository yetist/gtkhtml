INCLUDES = -I$(top_srcdir) -I$(srcdir) -I$(includedir) $(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS) \
	-DG_LOG_DOMAIN=\"gtkhtml\" $(LIBWWW_CFLAGS) \
	-DSRCDIR=\"$(srcdir)\"

lib_LTLIBRARIES = libgtkhtml.la

noinst_PROGRAMS = testgtkhtml

if BONOBO
bin_PROGRAMS = html-component
endif

libgtkhtmldir = $(includedir)/gtkhtml

libgtkhtml_HEADERS =				\
	debug.h					\
	gtkhtml.h				\
	htmlbullet.h				\
	htmlcluealigned.h			\
	htmlclueflow.h				\
	htmlclue.h				\
	htmlclueh.h				\
	htmlcluev.h				\
	htmlcolor.h				\
	htmlengine.h				\
	htmlfont.h				\
	htmlhspace.h				\
	htmlimage.h				\
	htmllist.h				\
	htmlobject.h				\
	htmlpainter.h				\
	htmlrule.h				\
	htmlsettings.h				\
	htmlstack.h				\
	htmltable.h				\
	htmltablecell.h				\
	htmltext.h				\
	htmltextmaster.c			\
	htmltextmaster.h			\
	htmltextslave.h				\
	htmltokenizer.h				\
	htmlvspace.h				\
	stringtokenizer.h

libgtkhtml_la_SOURCES =				\
	debug.c					\
	gtkhtml.c				\
	htmlbullet.c				\
	htmlclue.c				\
	htmlcluealigned.c			\
	htmlclueflow.c				\
	htmlclueh.c				\
	htmlcluev.c				\
	htmlcolor.c				\
	htmlengine.c				\
	htmlfont.c				\
	htmlhspace.c				\
	htmlimage.c				\
	htmllist.c				\
	htmlobject.c				\
	htmlpainter.c				\
	htmlrule.c				\
	htmlsettings.c				\
	htmlstack.c				\
	htmlstream.c				\
	htmltable.c				\
	htmltablecell.c				\
	htmltext.c				\
	htmltextmaster.c			\
	htmltextslave.c				\
	htmltokenizer.c				\
	htmlvspace.c				\
	stringtokenizer.c

testgtkhtml_SOURCES = 		\
	testgtkhtml.c

if LIBWWW
GLIBWWW=$(top_builddir)/glibwww/libglibwww.a $(LIBWWW_LIBS)
else
GLIBWWW=
endif

testgtkhtml_LDADD =				\
	libgtkhtml.la				\
	$(GLIBWWW)				\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(EXTRA_GNOME_LIBS)

if BONOBO

html_component_SOURCES =			\
	html-component.c

html_component_LDADD =				\
	libgtkhtml.la				\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(EXTRA_GNOME_LIBS)

endif

dist-hook:
	mkdir $(distdir)/tests
	cp -f $(srcdir)/tests/*.html $(distdir)/tests
	cp -f $(srcdir)/tests/*.jpg $(distdir)/tests
	cp -f $(srcdir)/
