NULL=

INCLUDES = -I$(top_srcdir) -I$(srcdir) -I$(includedir) $(GNOME_INCLUDEDIR) \
	$(EXTRA_GNOME_CFLAGS) \
	-DG_LOG_DOMAIN=\"gtkhtml\" $(GLIBWWW_CFLAGS) \
	-DSRCDIR=\"$(srcdir)\" \
	-DPREFIX=\"$(prefix)\"

CFLAGS +=  $(GCONF_CFLAGS)

lib_LTLIBRARIES = libgtkhtml.la

if GLIBWWW
noinst_PROGRAMS = testgtkhtml
else
noinst_PROGRAMS =
endif

libgtkhtmldir = $(includedir)/gtkhtml

# FIXME not all the stuff should be installed!

libgtkhtml_HEADERS =				\
	gtkhtml-embedded.h			\
	gtkhtml-enums.h				\
	gtkhtml-im.h				\
	gtkhtml-input.h				\
	gtkhtml-private.h			\
	gtkhtml-properties.h			\
	gtkhtml-search.h			\
	gtkhtml-stream.h			\
	gtkhtml-types.h				\
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
	htmlcolor.h				\
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
	htmlengine-edit-images.c		\
	htmlengine-edit-images.h		\
	htmlengine-edit-insert.h		\
	htmlengine-edit-movement.h		\
	htmlengine-edit-paste.h			\
	htmlengine-edit-rule.h			\
	htmlengine-edit-selection-updater.h	\
	htmlengine-edit-text.h			\
	htmlengine-print.h			\
	htmlengine-save.h			\
	htmlengine-search.h			\
	htmlengine.h				\
	htmlentity.h				\
	htmlenums.h				\
	htmlenumutils.h				\
	htmlfontmanager.h			\
	htmlform.h				\
	htmlgdkpainter.h			\
	htmlhidden.h				\
	htmlimage.h				\
	htmlimageinput.h			\
	htmlinterval.h				\
	htmllinktextmaster.h			\
	htmllist.h				\
	htmlobject.h				\
	htmlpainter.h				\
	htmlprinter.h				\
	htmlradio.h				\
	htmlrule.h				\
	htmlsearch.h				\
	htmlreplace.h				\
	htmlselect.h				\
	htmlselection.h				\
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
	htmltypes.h				\
	htmlundo.h				\
	htmlundo-action.h			\
	htmlurl.h				\
	htmlvspace.h				\
	gtkhtml-keybinding.h			\
	htmliframe.h				\
	htmlengine-edit-table.h			\
	htmlhspace.h				\
	$(NULL)

libgtkhtml_la_SOURCES =				\
	gtkhtml-embedded.c			\
	gtkhtml-im.c				\
	gtkhtml-input.c				\
	gtkhtml-properties.c			\
	gtkhtml-search.c			\
	gtkhtml-stream.c			\
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
	htmlcolor.c				\
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
	htmlengine-edit-images.c		\
	htmlengine-edit-insert.c		\
	htmlengine-edit-movement.c		\
	htmlengine-edit-paste.c			\
	htmlengine-edit-rule.c			\
	htmlengine-edit-selection-updater.c	\
	htmlengine-edit-text.c			\
	htmlengine-print.c			\
	htmlengine-save.c			\
	htmlengine-search.c			\
	htmlengine.c				\
	htmlentity.c				\
	htmlenumutils.c				\
	htmlfontmanager.c			\
	htmlform.c				\
	htmlgdkpainter.c			\
	htmlhidden.c				\
	htmlimage.c				\
	htmlimageinput.c			\
	htmlinterval.c				\
	htmllinktextmaster.c			\
	htmllist.c				\
	htmlobject.c				\
	htmlpainter.c				\
	htmlprinter.c				\
	htmlradio.c				\
	htmlrule.c				\
	htmlsearch.c				\
	htmlreplace.c				\
	htmlselect.c				\
	htmlselection.c				\
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
	htmlvspace.c				\
	htmliframe.c				\
	$(NULL)
#	htmlhspace.c				\
#	htmlelement.c				\
#	html-component.c			\
#	htmlengine-edit-table.c			\

libgtkhtml_la_LDFLAGS = -version-info $(GTKHTML_CURRENT):$(GTKHTML_REVISION):$(GTKHTML_AGE)

libgtkhtml_la_LIBADD = $(EXTRA_GNOME_LIBS) $(GCONF_LIBS)

# The test program.

testgtkhtml_SOURCES = 		\
	testgtkhtml.c		\
	htmlurl.c

testgtkhtml_CFLAGS = $(GLIBWWW_CFLAGS)
testgtkhtml_LDFLAGS += -static
testgtkhtml_LDADD =				\
	libgtkhtml.la				\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(EXTRA_GNOME_LIBS)                     \
	$(GLIBWWW_LIBS)				\
	$(GCONF_LIBS)
#	-lccmalloc -ldl
#

dist-hook:
	mkdir $(distdir)/tests
	cp -f $(srcdir)/tests/*.html $(distdir)/tests
	cp -f $(srcdir)/tests/*.jpg $(distdir)/tests

EXTRA_DIST = \
	keybindingsrc.emacs \
	keybindingsrc.ms

keybindingsdir = $(datadir)/gtkhtml
keybindings_DATA = \
	keybindingsrc.emacs \
	keybindingsrc.ms
