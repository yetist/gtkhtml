# Basic setup

gtkhtml_data = $(datadir)/gtkhtml-@GTKHTML_RELEASE@

gladedir = $(gtkhtml_data)
glade_DATA = gtkhtml-editor-properties.glade

INCLUDES = -I$(top_srcdir)/src -I$(srcdir) -I$(includedir) $(GTKHTML_CFLAGS) $(SOUP_CFLAGS) \
	-DG_LOG_DOMAIN=\"gtkhtml\" \
	-DSRCDIR=\"$(srcdir)\" \
	-DPREFIX=\"$(prefix)\"

AM_CFLAGS = \
	$(EDITOR_CFLAGS)				\
	-DICONDIR=\"$(gtkhtml_data)/icons\"		\
        -DGTKHTML_DATADIR=\""$(gtkhtml_data)"\"		\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGLADE_DATADIR=\"$(gladedir)\"			\
	-DGTK_DISABLE_DEPRECATED=1 			\
	-DGDK_DISABLE_DEPRECATED=1			\
	-DG_DISABLE_DEPRECATED=1			\
	-DGNOME_DISABLE_DEPRECATED=1			\
	-DPREFIX=\""$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBDIR=\""$(datadir)"\"			\
	-DBONOBO_DISABLE_DEPRECATED=1

# CORBA stuff

IDLS = Editor.idl

SPELL_IDLS = Spell.idl

IDL_GENERATED = 		\
	Editor.h		\
	Editor-common.c		\
	Editor-skels.c		\
	Editor-stubs.c

SPELL_IDL_GENERATED = 		\
	Spell.h			\
	Spell-common.c		\
	Spell-skels.c		\
	Spell-stubs.c

Editor-common.o: Editor.h
Spell-common.o: Spell.h

$(IDL_GENERATED): $(IDLS)
	$(ORBIT_IDL) $(BONOBO_IDL_INCLUDES) $(srcdir)/Editor.idl

$(SPELL_IDL_GENERATED): $(SPELL_IDLS)
	$(ORBIT_IDL) $(BONOBO_IDL_INCLUDES) $(srcdir)/Spell.idl

LDADD =	\
	$(top_builddir)/src/libgtkhtml-@GTKHTML_RELEASE@.la	\
	$(top_builddir)/a11y/libgtkhtml-a11y-@GTKHTML_RELEASE@.la	\
	$(EDITOR_LIBS)

# -lpspell-modules -lpspell_aspell -laspell -lpspell -lstdc++
#	-lccmalloc -ldl

# The Editor Control

libgnome_gtkhtml_editor_3_0_la: Editor.h

libgnome_gtkhtml_editor_3_0_la_SOURCES =	\
	$(SPELL_IDL_GENERATED)			\
	Editor.h				\
	body.h					\
	body.c					\
	cell.h					\
	cell.c					\
	control-data.c				\
	control-data.h				\
	dialog.c				\
	dialog.h				\
	editor-control-factory.c		\
	editor-control-factory.h		\
	engine.h				\
	engine.c				\
	html-stream-mem.h			\
	html-stream-mem.c			\
	image.c					\
	image.h					\
	link.c					\
	link.h					\
	menubar.c				\
	menubar.h				\
	paragraph.c				\
	paragraph.h				\
	persist-file.c				\
	persist-file.h				\
	persist-stream.c			\
	persist-stream.h			\
	popup.c					\
	popup.h					\
	properties.c				\
	properties.h				\
	replace.c				\
	replace.h				\
	rule.c					\
	rule.h					\
	search.c				\
	search.h				\
	spell.c					\
	spell.h					\
	table.c					\
	table.h					\
	template.c				\
	template.h				\
	text.c					\
	text.h					\
	toolbar.c				\
	toolbar.h				\
	utils.c					\
	utils.h

if EDITOR_SHLIB
MY_PRGS =
MY_NOINSTPRGS =
MY_NOINSTLIBS =
else
MY_PRGS = gnome-gtkhtml-editor-3.0
MY_NOINSTPRGS =
MY_NOINSTLIBS = libgnome-gtkhtml-editor-3.0.la
endif

gnome_gtkhtml_editor_3_0_LDADD = \
	$(LDADD)			\
	libgnome-gtkhtml-editor-3.0.la

gnome_gtkhtml_editor_3_0_SOURCES = \
	$(IDL_GENERATED)		\
	gnome-gtkhtml-editor.c

noinst_LTLIBRARIES = $(MY_NOINSTLIBS)

noinst_PROGRAMS = 		\
	test_editor		\
	$(MY_NOINSTPRGS)

libexec_PROGRAMS = $(MY_PRGS)

serverdir = $(libdir)/bonobo/servers
server_in_files = GNOME_GtkHTML_Editor.server.in.in

server_DATA = $(server_in_files:.server.in.in=.server)
$(server_in_files:.server.in.in=.server.in): $(server_in_files) Makefile
	sed -e "s|\@GNOME_GTKHTML_EDITOR_LOCATION\@|$(GNOME_GTKHTML_EDITOR_LOCATION)|g" \
	-e "s|\@EDITOR_API_VERSION\@|$(EDITOR_API_VERSION)|g" \
	-e "s|\@GNOME_GTKHTML_EDITOR_TYPE\@|$(GNOME_GTKHTML_EDITOR_TYPE)|g" $< > $@

@INTLTOOL_SERVER_RULE@

GNOME_GtkHTML_Editor.xml: GNOME_GtkHTML_Editor.xml.in Makefile
	sed -e 's|\@ICONSDIR\@|$(gtkhtml_data)/icons|g' $< > $@

uidir = $(gtkhtml_data)
ui_DATA = GNOME_GtkHTML_Editor.xml

# The Test program

test_editor_SOURCES =				\
	$(IDL_GENERATED)			\
	e-html-utils.c				\
	e-html-utils.h				\
	htmlsourceview.c			\
	htmlsourceview.h			\
	test-html-editor-control.c

# Extra stuff to distribute.

EXTRA_DIST =					\
	$(IDLS)					\
	$(SPELL_IDLS)				\
	ChangeLog				\
	$(ui_DATA)				\
	GNOME_GtkHTML_Editor.xml.in		\
	gtkhtml-editor-properties.glade		\
	$(server_in_files) $(server_DATA)

idldir = $(gtkhtml_data)
idl_DATA = $(IDLS)

CLEANFILES = $(IDL_GENERATED) $(SPELL_IDL_GENERATED) 

BUILT_SOURCE = $(IDL_GENERATED) $(SPELL_IDL_GENERATED)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)
