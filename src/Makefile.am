INCLUDES = -I$(top_srcdir) -I$(includedir) $(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS) \
	-DG_LOG_DOMAIN=\"gtkhtml\"

if BONOBO
hc=html-component
endif

bin_PROGRAMS = testgtkhtml $(hc)

common_SOURCES =   \
	debug.c		\
	debug.h		\
	gtkhtml.c       \
	gtkhtml.h	\
	htmlbullet.c	\
	htmlbullet.h	\
	htmlclue.c	\
	htmlclueflow.c	\
	htmlclueflow.h	\
	htmlclue.h	\
	htmlclueh.c	\
	htmlclueh.h	\
	htmlcluev.c	\
	htmlcluev.h	\
	htmlcolor.c	\
	htmlcolor.h	\
	htmlengine.c	\
	htmlengine.h	\
	htmlfont.c	\
	htmlfont.h	\
	htmlhspace.c	\
	htmlhspace.h	\
	htmlimage.c     \
	htmlimage.h	\
	htmllist.c      \
	htmllist.h      \
	htmlobject.c	\
	htmlobject.h	\
	htmlpainter.c	\
	htmlpainter.h	\
	htmlrule.c	\
	htmlrule.h	\
	htmlsettings.c	\
	htmlsettings.h	\
	htmlstack.c	\
	htmlstack.h	\
	htmltable.c	\
	htmltable.h	\
	htmltablecell.c	\
	htmltablecell.h	\
	htmltext.c	\
	htmltext.h	\
	htmltextmaster.c\
	htmltextmaster.h\
	htmltextslave.c	\
	htmltextslave.h	\
	htmltokenizer.c	\
	htmltokenizer.h	\
	htmlvspace.c	\
	htmlvspace.h	\
	stringtokenizer.c	\
	stringtokenizer.h

testgtkhtml_SOURCES = 		\
	$(common_SOURCES) 	\
	testgtkhtml.c

testgtkhtml_LDADD = 		\
	$(GNOMEUI_LIBS) 	\
	$(GNOME_LIBDIR) 	\
	$(EXTRA_GNOME_LIBS)

if BONOBO
html_component_SOURCES = \
	$(common_SOURCES) 	\
	html-component.c

html_component_LDADD = 		\
	$(GNOMEUI_LIBS) 	\
	$(GNOME_LIBDIR) 	\
	$(EXTRA_GNOME_LIBS)
endif

dist-hook:
	mkdir $(distdir)/tests
	cp -f $(srcdir)/tests/*.html $(distdir)/tests
	cp -f $(srcdir)/tests/*.jpg $(distdir)/tests
	cp -f $(srcdir)/
