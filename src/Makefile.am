INCLUDES = -I$(top_srcdir) -I$(srcdir) -I$(includedir) $(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS) \
	-DG_LOG_DOMAIN=\"gtkhtml\" $(LIBWWW_CFLAGS) \
	-DSRCDIR=\"$(srcdir)\"

lib_LTLIBRARIES = libgtkhtml.la

noinst_PROGRAMS = testgtkhtml

libgtkhtmldir = $(includedir)/gtkhtml

libgtkhtml_HEADERS =				\
	debug.h					\
	gtkhtml-embedded.h			\
	gtkhtml.h				\
	htmlanchor.h				\
	htmlbutton.h				\
	htmlcheckbox.h				\
	htmlclue.h				\
	htmlcluealigned.h			\
	htmlclueflow.h				\
	htmlclueh.h				\
	htmlcluev.h				\
	htmlcolorset.h				\
	htmlcursor.h				\
	htmldrawqueue.h				\
	htmlembedded.h				\
	htmlengine-cutbuffer.h			\
	htmlengine-edit.h			\
	htmlengine-edit-clueflowstyle.h		\
	htmlengine-edit-copy.h			\
	htmlengine-edit-cut.h			\
	htmlengine-edit-paste.h			\
	htmlengine-print.h			\
	htmlengine-save.h			\
	htmlengine.h				\
	htmlentity.h				\
	htmlgdkfontmanager.h			\
	htmlfontstyle.h				\
	htmlform.h				\
	htmlgdkpainter.h			\
	htmlhidden.h				\
	htmlimage.h				\
	htmlimageinput.h			\
	htmllinktext.h				\
	htmllinktextmaster.h			\
	htmllist.h				\
	htmlobject.h				\
	htmlpainter.h				\
	htmlprinter.h				\
	htmlprintfontmanager.h			\
	htmlradio.h				\
	htmlrule.h				\
	htmlselect.h				\
	htmlsettings.h				\
	htmlstack.h				\
	htmltable.h				\
	htmltablecell.h				\
	htmltext.h				\
	htmltextarea.h				\
	htmltextinput.h				\
	htmltextmaster.c			\
	htmltextmaster.h			\
	htmltextslave.h				\
	htmltokenizer.h				\
	htmltype.h				\
	htmlundo.h				\
	htmlundo-action.h			\
	htmlurl.h				\
	htmlvspace.h				\
	stringtokenizer.h

libgtkhtml_la_SOURCES =				\
	debug.c					\
	gtkhtml-embedded.c			\
	gtkhtml-private.h			\
	gtkhtml.c				\
	htmlanchor.c				\
	htmlbutton.c				\
	htmlcheckbox.c				\
	htmlclue.c				\
	htmlcluealigned.c			\
	htmlclueflow.c				\
	htmlclueh.c				\
	htmlcluev.c				\
	htmlcolorset.c				\
	htmlcursor.c				\
	htmldrawqueue.c				\
	htmlembedded.c				\
	htmlengine-cutbuffer.c			\
	htmlengine-edit.c			\
	htmlengine-edit-clueflowstyle.c		\
	htmlengine-edit-copy.c			\
	htmlengine-edit-cut.c			\
	htmlengine-edit-paste.c			\
	htmlengine-print.c			\
	htmlengine-save.c			\
	htmlengine.c				\
	htmlentity.c				\
	htmlgdkfontmanager.c			\
	htmlfontstyle.c				\
	htmlform.c				\
	htmlgdkpainter.c			\
	htmlhidden.c				\
	htmlimage.c				\
	htmlimageinput.c			\
	htmllinktext.c				\
	htmllinktextmaster.c			\
	htmllist.c				\
	htmlobject.c				\
	htmlpainter.c				\
	htmlprinter.c				\
	htmlprintfontmanager.c			\
	htmlradio.c				\
	htmlrule.c				\
	htmlselect.c				\
	htmlsettings.c				\
	htmlstack.c				\
	htmlstream.c				\
	htmltable.c				\
	htmltablecell.c				\
	htmltext.c				\
	htmltextarea.c				\
	htmltextinput.c				\
	htmltextmaster.c			\
	htmltextslave.c				\
	htmltokenizer.c				\
	htmltype.c				\
	htmlundo.c				\
	htmlundo-action.c			\
	htmlvspace.c				\
	stringtokenizer.c

testgtkhtml_SOURCES = 		\
	testgtkhtml.c		\
	htmlurl.c

if LIBWWW
GLIBWWW=$(top_builddir)/glibwww/libglibwww.a $(LIBWWW_LIBS)
else
GLIBWWW=
endif

testgtkhtml_LDADD =				\
	libgtkhtml.la				\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(EXTRA_GNOME_LIBS)                     \
	$(GLIBWWW)

dist-hook:
	mkdir $(distdir)/tests
	cp -f $(srcdir)/tests/*.html $(distdir)/tests
	cp -f $(srcdir)/tests/*.jpg $(distdir)/tests
