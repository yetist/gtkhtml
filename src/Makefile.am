INCLUDES = -I$(top_srcdir) -I$(srcdir) -I$(includedir) $(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS) \
	-DG_LOG_DOMAIN=\"gtkhtml\" $(LIBWWW_CFLAGS) \
	-DSRCDIR=\"$(srcdir)\"

lib_LTLIBRARIES = libgtkhtml.la

if LIBWWW
noinst_PROGRAMS = testgtkhtml
else
noinst_PROGRAMS =
endif

libgtkhtmldir = $(includedir)/gtkhtml

# FIXME not all the stuff should be installed!

libgtkhtml_HEADERS =				\
	gtkhtml-edit-image.h			\
	gtkhtml-embedded.h			\
	gtkhtml-keybinding.h			\
	gtkhtml-private.h			\
	gtkhtml-search.h			\
	gtkhtml-stream.h			\
	gtkhtml-replace.h			\
	gtkhtml.h				\
	gtkhtmldebug.h				\
	gtkhtmlfontstyle.h			\
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
	htmlengine-edit-cursor.h		\
	htmlengine-edit-cut.h			\
	htmlengine-edit-delete.h		\
	htmlengine-edit-fontstyle.h		\
	htmlengine-edit-insert.h		\
	htmlengine-edit-movement.h		\
	htmlengine-edit-paste.h			\
	htmlengine-edit-selection-updater.h	\
	htmlengine-print.h			\
	htmlengine-save.h			\
	htmlengine.h				\
	htmlentity.h				\
	htmlgdkfontmanager.h			\
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
	htmlsearch.h				\
	htmlreplace.h				\
	htmlselect.h				\
	htmlsettings.h				\
	htmlstack.h				\
	htmlstringtokenizer.h			\
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
	htmlvspace.h

libgtkhtml_la_SOURCES =				\
	gtkhtml-edit-image.c			\
	gtkhtml-embedded.c			\
	gtkhtml-keybinding.c			\
	gtkhtml-search.c			\
	gtkhtml-stream.c			\
	gtkhtml-replace.c			\
	gtkhtml.c				\
	gtkhtmldebug.c				\
	gtkhtmlfontstyle.c			\
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
	htmlengine-edit-cursor.c		\
	htmlengine-edit-cut.c			\
	htmlengine-edit-delete.c		\
	htmlengine-edit-fontstyle.c		\
	htmlengine-edit-insert.c		\
	htmlengine-edit-movement.c		\
	htmlengine-edit-paste.c			\
	htmlengine-edit-selection-updater.c	\
	htmlengine-print.c			\
	htmlengine-save.c			\
	htmlengine.c				\
	htmlentity.c				\
	htmlgdkfontmanager.c			\
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
	htmlsearch.c				\
	htmlreplace.c				\
	htmlselect.c				\
	htmlsettings.c				\
	htmlstack.c				\
	htmlstringtokenizer.c			\
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
	htmlvspace.c

libgtkhtml_la_LDFLAGS = -version-info $(GTKHTML_CURRENT):$(GTKHTML_REVISION):$(GTKHTML_AGE)

# The test program.

testgtkhtml_SOURCES = 		\
	testgtkhtml.c		\
	htmlurl.c

GLIBWWW=$(top_builddir)/glibwww/libglibwww.a $(LIBWWW_LIBS)

testgtkhtml_LDFLAGS += -static
testgtkhtml_LDADD =				\
	libgtkhtml.la				\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(EXTRA_GNOME_LIBS)                     \
	$(GLIBWWW)
# -lccmalloc -ldl
#

dist-hook:
	mkdir $(distdir)/tests
	cp -f $(srcdir)/tests/*.html $(distdir)/tests
	cp -f $(srcdir)/tests/*.jpg $(distdir)/tests
