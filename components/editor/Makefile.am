### Process this file with automake to produce Makefile.in

gtkhtml_data = $(datadir)/gtkhtml-@GTKHTML_API_VERSION@

bin_PROGRAMS = gtkhtml-editor-test
lib_LTLIBRARIES = libgtkhtml-editor-4.0.la

AM_CFLAGS = \
	@GTKHTML_CFLAGS@			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/gtkhtml			\
	-I$(top_srcdir)/components/editor	\
	-DG_LOG_DOMAIN=\"gtkhtml-editor\"	\
	-DDATADIR=\""$(datadir)"\"		\
	-DLOCALEDIR=\""$(datadir)/locale/"\"	\
	-DICONDIR=\""$(gtkhtml_data)/icons"\"	\
	-DHAVE_CONFIG_H

BUILT_SOURCES = \
	gtkhtml-editor-marshal.c		\
	gtkhtml-editor-marshal.h		\
        gtkhtml-resources.h			\
        gtkhtml-resources.c			\
	gtkhtml-spell-marshal.c			\
	gtkhtml-spell-marshal.h

libgtkhtml_editordir = $(includedir)/libgtkhtml-@GTKHTML_API_VERSION@/editor

libgtkhtml_editor_4_0_la_SOURCES = \
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
	gtkhtml-editor-private.h		\
	gtkhtml-editor-signals.c		\
	gtkhtml-face.c				\
	gtkhtml-face.h				\
	gtkhtml-face-action.c			\
	gtkhtml-face-action.h			\
	gtkhtml-face-chooser.c			\
	gtkhtml-face-chooser.h			\
	gtkhtml-face-chooser-menu.c		\
	gtkhtml-face-chooser-menu.h		\
	gtkhtml-face-tool-button.c		\
	gtkhtml-face-tool-button.h		\
	gtkhtml-image-chooser-dialog.c		\
	gtkhtml-image-chooser-dialog.h		\
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

libgtkhtml_editor_4_0_la_LIBADD = \
	@GTKHTML_LIBS@				\
	$(top_builddir)/gtkhtml/libgtkhtml-@GTKHTML_API_VERSION@.la

libgtkhtml_editor_4_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO) -no-undefined

gtkhtml_editor_test_SOURCES = main.c

gtkhtml_editor_test_LDADD = \
	@GTKHTML_LIBS@				\
	libgtkhtml-editor-@EDITOR_API_VERSION@.la			\
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

gtkhtml-resources.c: gtkhtml.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gtkhtml.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --target=$@ --generate-source --c-name gtkhtml $(srcdir)/gtkhtml.gresource.xml

gtkhtml-resources.h: gtkhtml.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gtkhtml.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --target=$@ --generate-header --c-name gtkhtml $(srcdir)/gtkhtml.gresource.xml

%-$(EDITOR_API_VERSION).pc: %.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_in_files = gtkhtml-editor.pc.in
pkgconfig_DATA = $(pkgconfig_in_files:.pc.in=-$(EDITOR_API_VERSION).pc)

uidir = $(gtkhtml_data)
ui_DATA = \
	gtkhtml-editor-builder.ui		\
	gtkhtml-editor-manager.ui

EXTRA_DIST = \
	$(pkgconfig_DATA)			\
	$(ui_DATA)				\
	gtkhtml-editor-marshal.list		\
	gtkhtml-spell-marshal.list		\
	gtkhtml.gresource.xml			\
	gtkhtml-editor.xml

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(pkgconfig_DATA)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

if GLADE_CATALOG

gladecatalog_DATA = gtkhtml-editor.xml
gladecatalogdir = `$(PKG_CONFIG) --variable=catalogdir gladeui-2.0`

libglade_gtkhtml_editor_LTLIBRARIES = libglade-gtkhtml-editor.la
libglade_gtkhtml_editordir = `$(PKG_CONFIG) --variable=moduledir gladeui-2.0`

libglade_gtkhtml_editor_la_CFLAGS = @GTKHTML_CFLAGS@

libglade_gtkhtml_editor_la_SOURCES = \
	gtkhtml-color-combo.c			\
	gtkhtml-color-palette.c			\
	gtkhtml-color-state.c			\
	gtkhtml-color-swatch.c			\
	gtkhtml-combo-box.c

libglade_gtkhtml_editor_la_LDFLAGS = -module -avoid-version -no-undefined

libglade_gtkhtml_editor_la_LIBADD = @GTKHTML_CFLAGS@

endif

-include $(top_srcdir)/git.mk
