# Basic setup

INCLUDES =						\
        -DGNOMEDATADIR=\""$(datadir)"\"		\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)/src				\
	-I$(srcdir)					\
	-I.						\
        $(GNOME_INCLUDEDIR)				\
	$(EXTRA_GNOME_CFLAGS)				\
	$(GTK_CFLAGS)

CFLAGS +=  $(GCONF_CFLAGS) $(PSPELL_CFLAGS)

# CORBA stuff

IDLS = \
	Editor.idl

SPELL_IDLS = \
	Spell.idl

IDL_GENERATED = \
	Editor.h		\
	Editor-common.c	\
	Editor-skels.c	\
	Editor-stubs.c

SPELL_IDL_GENERATED = \
	Spell.h			\
	Spell-common.c		\
	Spell-skels.c		\
	Spell-stubs.c

CLEANFILES = $(IDL_GENERATED) $(SPELL_IDL_GENERATED)

Editor-impl.o: Editor.h
Spell-impl.o: Spell.h

$(IDL_GENERATED): $(IDLS)
	$(ORBIT_IDL) -I$(datadir)/idl -I`$(GNOME_CONFIG) --cflags idl`	\
		-I$(srcdir) $(srcdir)/Editor.idl

$(SPELL_IDL_GENERATED): $(SPELL_IDLS)
	$(ORBIT_IDL) -I$(datadir)/idl -I`$(GNOME_CONFIG) --cflags idl`	\
		-I$(srcdir) $(srcdir)/Spell.idl

LDFLAGS += -dynamic

LDADD =	\
	$(top_builddir)/src/libgtkhtml.la	\
	$(EXTRA_GNOME_LIBS)			\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBS)				\
	$(GCONF_LIBS)

# -lpspell-modules -lpspell_aspell -laspell -lpspell -lstdc++
#	-lccmalloc -ldl

# The Editor Control

bin_PROGRAMS =					\
	gnome-gtkhtml-editor

gnome_gtkhtml_editor_SOURCES =			\
	$(IDL_GENERATED)			\
	$(SPELL_IDL_GENERATED)			\
	body.h					\
	body.c					\
	control-data.c				\
	control-data.h				\
	dialog.c				\
	dialog.h				\
	editor-control-factory.c		\
	editor-control-factory.h		\
	engine.h				\
	engine.c				\
	image.c					\
	image.h					\
	link.c					\
	link.h					\
	main.c					\
	menubar.c				\
	menubar.h				\
	paragraph.c				\
	paragraph.h				\
	persist-file-impl.c			\
	persist-file-impl.h			\
	persist-stream-impl.c			\
	persist-stream-impl.h			\
	popup.c					\
	popup.h					\
	properties.c				\
	properties.h				\
	replace.c				\
	replace.h				\
	resolver-progressive-impl.c		\
	resolver-progressive-impl.h		\
	rule.c					\
	rule.h					\
	search.c				\
	search.h				\
	spell.c					\
	spell.h					\
	text.c					\
	text.h					\
	toolbar.c				\
	toolbar.h				\
	utils.c					\
	utils.h
#	table.c					\
#	table.h					\

OAF_INFO = GNOME_GtkHTML_Editor.oafinfo
GNORBA_INFO = html-editor-control.gnorba

if USING_OAF
oafdir = $(datadir)/oaf
oaf_DATA = $(OAF_INFO)
else
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = $(GNORBA_INFO)
endif

uidir = $(datadir)/gnome/ui
ui_DATA = GNOME_GtkHTML_Editor.xml

# The Test program

noinst_PROGRAMS =				\
	test-editor

test_editor_SOURCES =				\
	$(IDL_GENERATED)			\
	resolver.c				\
	resolver.h				\
	test-html-editor-control.c

# Extra stuff to distribute.

EXTRA_DIST =					\
	$(IDLS)					\
	$(SPELL_IDLS)				\
	$(GNORBA_INFO)				\
	$(OAF_INFO)				\
	ChangeLog				\
	tt.xpm					\
	$(ui_DATA)

idldir = $(datadir)/gtkhtml
idl_DATA = $(IDLS)
