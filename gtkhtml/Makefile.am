if OS_WIN32
PLATFORM_DEP_SOURCES = dllmain.c
else
PLATFORM_DEP_SOURCES = 
endif

NULL=

gtkhtml_data = $(datadir)/gtkhtml-@GTKHTML_API_VERSION@

if HAVE_NEWSOUP
have_new_soup = -DHAVE_NEWSOUP
endif

AM_CPPFLAGS = 				\
	-I$(top_srcdir)			\
	-I$(srcdir)			\
	$(GTKHTML_CFLAGS)		\
	$(SOUP_CFLAGS)			\
	$(I18N_LIB_CFLAGS)		\
	-DG_LOG_DOMAIN=\"gtkhtml\"	\
	${have_new_soup}

AM_CFLAGS = 						\
	-DPREFIX=\""$(prefix)"\"			\
	-DLIBDIR=\""$(datadir)"\"			\
	-DDATADIR=\""$(datadir)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DICONDIR=\"$(gtkhtml_data)/icons\"		\
	-DGTKHTML_DATADIR=\""$(gtkhtml_data)"\"		\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

libgtkhtmldir = $(includedir)/libgtkhtml-@GTKHTML_API_VERSION@/gtkhtml

lib_LTLIBRARIES = libgtkhtml-4.0.la

# FIXME not all the stuff should be installed!

libgtkhtml_HEADERS =				\
	gtkhtml-embedded.h			\
	gtkhtml-enums.h				\
	gtkhtml-properties.h			\
	gtkhtml-search.h			\
	gtkhtml-stream.h			\
	gtkhtml-types.h				\
	gtkhtml-version.h				\
	gtkhtml.h				\
	htmltypes.h				\
	htmlenums.h				\
	htmltokenizer.h				\
	$(NULL)

libgtkhtml_4_0_la_SOURCES =			\
	gtkhtml-embedded.c			\
	gtkhtml-properties.c			\
	gtkhtml-search.c			\
	gtkhtml-stream.c			\
	gtkhtml-version.c			\
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
	htmlengine-edit.c			\
	htmlengine-edit-clueflowstyle.c		\
	htmlengine-edit-cursor.c		\
	htmlengine-edit-cut-and-paste.c		\
	htmlengine-edit-fontstyle.c		\
	htmlengine-edit-images.c		\
	htmlengine-edit-movement.c		\
	htmlengine-edit-rule.c			\
	htmlengine-edit-selection-updater.c	\
	htmlengine-edit-table.c			\
	htmlengine-edit-tablecell.c		\
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
	htmlplainpainter.c			\
	htmlhidden.c				\
	htmlimage.c				\
	htmlimageinput.c			\
	htmlinterval.c				\
	htmllist.c				\
	htmlmap.c				\
	htmlmarshal.c				\
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
	htmlshape.c				\
	htmlstack.c				\
	htmlstringtokenizer.c			\
	htmlstyle.c				\
	htmltable.c				\
	htmltablecell.c				\
	htmltext.c				\
	htmltextarea.c				\
	htmltextinput.c				\
	htmltextslave.c				\
	htmltokenizer.c				\
	htmltype.c				\
	htmlundo.c				\
	htmlundo-action.c			\
	htmliframe.c				\
	htmlframe.c				\
	htmlframeset.c				\
	gtkhtml-compat.h			\
	gtkhtml-keybinding.h			\
	gtkhtml-private.h			\
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
	htmlengine-edit.h			\
	htmlengine-edit-clueflowstyle.h		\
	htmlengine-edit-cursor.h		\
	htmlengine-edit-cut-and-paste.h         \
	htmlengine-edit-fontstyle.h		\
	htmlengine-edit-images.h		\
	htmlengine-edit-movement.h		\
	htmlengine-edit-rule.h			\
	htmlengine-edit-selection-updater.h	\
	htmlengine-edit-table.h			\
	htmlengine-edit-tablecell.h		\
	htmlengine-edit-text.h			\
	htmlengine-print.h			\
	htmlengine-save.h			\
	htmlengine-search.h			\
	htmlengine.h				\
	htmlentity.h				\
	htmlenumutils.h				\
	htmlfontmanager.h			\
	htmlform.h				\
	htmlgdkpainter.h			\
	htmlplainpainter.h			\
	htmlhidden.h				\
	htmlimage.h				\
	htmlimageinput.h			\
	htmlinterval.h				\
	htmllist.h				\
	htmlmap.h				\
	htmlmarshal.h				\
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
	htmlshape.h				\
	htmlstack.h				\
	htmlstringtokenizer.h			\
	htmlstyle.h				\
	htmltable.h				\
	htmltablepriv.h				\
	htmltablecell.h				\
	htmltext.h				\
	htmltextarea.h				\
	htmltextinput.h				\
	htmltextslave.h				\
	htmltype.h				\
	htmlundo.h				\
	htmlundo-action.h			\
	htmliframe.h				\
	htmlframe.h				\
	htmlframeset.h				\
	$(PLATFORM_DEP_SOURCES)			\
	$(NULL)

libgtkhtml_4_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO) -no-undefined

libgtkhtml_4_0_la_LIBADD =			\
	$(GTKHTML_LIBS)				\
	../a11y/libgtkhtml-a11y.la		\
	$(GAIL_LIBS)				\
	$(REGEX_LIBS)				\
	$(LIBM)

dist-hook:
	mkdir $(distdir)/tests
	cp -f $(srcdir)/tests/*.html $(distdir)/tests
	cp -f $(srcdir)/tests/*.jpg $(distdir)/tests

if HAVE_SOUP
test_programs = testgtkhtml
endif
noinst_PROGRAMS = $(test_programs) gtest test-suite test-stress


testgtkhtml_SOURCES = 		\
	testgtkhtml.c		\
	htmlurl.h		\
	htmlurl.c
#testgtkhtml_LDFLAGS = -static
testgtkhtml_LDADD =					\
	libgtkhtml-@GTKHTML_API_VERSION@.la			\
	$(GTKHTML_LIBS)					\
	$(SOUP_LIBS)

gtest_SOURCES = \
	test.c
gtest_LDFLAGS =
gtest_LDADD =	\
	libgtkhtml-@GTKHTML_API_VERSION@.la			\
	$(GTKHTML_LIBS)

%-@GTKHTML_API_VERSION@.pc: %.pc
	 cp $< $@

test_suite_SOURCES = \
	test-suite.c
test_suite_LDFLAGS =
test_suite_LDADD =	\
	libgtkhtml-@GTKHTML_API_VERSION@.la			\
	$(GTKHTML_LIBS)

test_stress_SOURCES = \
	test-stress.c
test_stress_LDFLAGS =
test_stress_LDADD =	\
	libgtkhtml-@GTKHTML_API_VERSION@.la			\
	$(GTKHTML_LIBS)

pkgconfig_in_files = libgtkhtml.pc.in
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pkgconfig_in_files:.pc.in=-@GTKHTML_API_VERSION@.pc)

keybindingsdir = $(datadir)/gtkhtml-@GTKHTML_API_VERSION@
keybindings_DATA =		\
	keybindingsrc.emacs

DISTCLEANFILES = libgtkhtml-@GTKHTML_API_VERSION@.pc

EXTRA_DIST =			\
	htmlclosures.list	\
	$(keybindings_DATA)	\
	$(NULL)

htmlmarshal.h:	htmlclosures.list
	glib-genmarshal --header --prefix=html_g_cclosure_marshal $< > $@

htmlmarshal.c:	htmlclosures.list
	( echo '#include "htmlmarshal.h"' > $@ &&				\
	glib-genmarshal --body --prefix=html_g_cclosure_marshal $< >> $@ ) || rm -f $@

test:	test-suite
	./test-suite > /dev/null

test-verbose:	test-suite
	./test-suite

test-memcheck:	test-suite
	./test-suite > /dev/null 2>&1 # 1st run to make sure .libs/lt-test-suite is available
	valgrind --num-callers=64 --tool=memcheck --run-libc-freeres=no --alignment=8 --db-attach=no .libs/lt-test-suite

stress-test:	test-stress
	./test-stress > /dev/null

stress-test-memcheck:	test-stress
	./test-stress dont-run > /dev/null 2>&1 # 1st run to make sure .libs/lt-test-suite is available
	valgrind --num-callers=64 --tool=memcheck --run-libc-freeres=no --alignment=8 --db-attach=no .libs/lt-test-stress

-include $(top_srcdir)/git.mk
