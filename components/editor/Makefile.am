### Process this file with automake to produce Makefile.in

gtkhtml_data = $(datadir)/gtkhtml-@GTKHTML_API_VERSION@

bin_PROGRAMS = gtkhtml-editor-test
lib_LTLIBRARIES = libgtkhtml-editor.la

AM_CFLAGS = \
	@GTKHTML_CFLAGS@			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/gtkhtml			\
	-I$(top_srcdir)/components/editor	\
	-DG_LOG_DOMAIN=\"gtkhtml-editor\"	\
	-DLOCALEDIR=\""$(datadir)/locale/"\"	\
	-DICONDIR=\""$(gtkhtml_data)/icons"\"	\
	-DHAVE_CONFIG_H

BUILT_SOURCES = \
	gtkhtml-editor-marshal.c		\
	gtkhtml-editor-marshal.h		\
	gtkhtml-spell-marshal.c			\
	gtkhtml-spell-marshal.h

libgtkhtml_editordir = $(includedir)/libgtkhtml-@GTKHTML_API_VERSION@/editor

libgtkhtml_editor_la_SOURCES = \
	$(BUILT_SOURCES)			\
	gtkhtml-color-combo.c			\
	gtkhtml-color-combo.h			\
	gtkhtml-color-palette.c			\
	gtkhtml-color-palette.h			\
	gtkhtml-color-state.c			\
	gtkhtml-color-state.h			\
	gtkhtml-color-swatch.c			\
	gtkhtml-color-swatch.h			\
	gtkhtml-combo-box.c			\
	gtkhtml-combo-box.h			\
	gtkhtml-editor.c			\
	gtkhtml-editor-actions.c		\
	gtkhtml-editor-private.c		\
	gtkhtml-editor-private.h		\
	gtkhtml-editor-signals.c		\
	gtkhtml-editor-signals.h		\
	gtkhtml-spell-dialog.c			\
	gtkhtml-spell-dialog.h			\
	gtkhtml-spell-checker.c			\
	gtkhtml-spell-language.c

libgtkhtml_editor_HEADERS = \
	gtkhtml-editor.h			\
	gtkhtml-editor-actions.h		\
	gtkhtml-editor-common.h			\
	gtkhtml-editor-widgets.h		\
	gtkhtml-spell-checker.h			\
	gtkhtml-spell-language.h

libgtkhtml_editor_la_LIBADD = \
	@GTKHTML_LIBS@				\
	$(top_builddir)/gtkhtml/libgtkhtml-@GTKHTML_API_VERSION@.la

gtkhtml_editor_test_SOURCES = main.c

gtkhtml_editor_test_LDADD = \
	libgtkhtml-editor.la			\
	$(top_builddir)/gtkhtml/libgtkhtml-@GTKHTML_API_VERSION@.la

gtkhtml-editor-marshal.h: gtkhtml-editor-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gtkhtml_editor_marshal > $@

gtkhtml-editor-marshal.c: gtkhtml-editor-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gtkhtml-editor-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gtkhtml_editor_marshal >> $@

gtkhtml-spell-marshal.h: gtkhtml-spell-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=gtkhtml_spell_marshal > $@

gtkhtml-spell-marshal.c: gtkhtml-spell-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gtkhtml-spell-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gtkhtml_spell_marshal >> $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gtkhtml-editor.pc

uidir = $(gtkhtml_data)
ui_DATA = \
	gtkhtml-editor.glade			\
	gtkhtml-editor.ui

EXTRA_DIST = \
	ChangeLog				\
	$(pkgconfig_DATA)			\
	$(ui_DATA)				\
	gtkhtml-editor-marshal.list		\
	gtkhtml-spell-marshal.list

CLEANFILES = \
	$(BUILT_SOURCES)

MAINTAINERCLEANFILES = \
	gtkhtml-editor.glade.bak		\
	gtkhtml-editor.gladep.bak

if MAINTAINER_MODE

gladecatalog_DATA = gtkhtml-editor.xml
gladecatalogdir = `$(PKG_CONFIG) --variable=catalogdir gladeui-1.0`

libglade_gtkhtml_editor_LTLIBRARIES = libglade-gtkhtml-editor.la
libglade_gtkhtml_editordir = `$(PKG_CONFIG) --variable=moduledir gladeui-1.0`

libglade_gtkhtml_editor_la_CFLAGS = @GTKHTML_CFLAGS@

libglade_gtkhtml_editor_la_SOURCES = \
	gtkhtml-color-combo.c			\
	gtkhtml-color-palette.c			\
	gtkhtml-color-state.c			\
	gtkhtml-color-swatch.c			\
	gtkhtml-combo-box.c

libglade_gtkhtml_editor_la_LDFLAGS = -module -avoid-version

libglade_gtkhtml_editor_la_LIBADD = @GTKHTML_CFLAGS@

EXTRA_DIST += $(gladecatalog_DATA)

endif
