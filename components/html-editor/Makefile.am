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
	HTMLEditor.idl

SPELL_IDLS = \
	Spell.idl

IDL_GENERATED = \
	HTMLEditor.h		\
	HTMLEditor-common.c	\
	HTMLEditor-skels.c	\
	HTMLEditor-stubs.c

SPELL_IDL_GENERATED = \
	Spell.h			\
	Spell-common.c		\
	Spell-skels.c		\
	Spell-stubs.c

CLEANFILES = $(IDL_GENERATED) $(SPELL_IDL_GENERATED)

HTMLEditor-impl.o: HTMLEditor.h

$(IDL_GENERATED): $(IDLS)
	$(ORBIT_IDL) -I$(datadir)/idl -I`$(GNOME_CONFIG) --cflags idl`	\
		-I$(srcdir) $(srcdir)/HTMLEditor.idl

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
	html-editor-control

html_editor_control_SOURCES =			\
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

if USING_OAF
oafdir = $(datadir)/oaf
oaf_DATA = html-editor-control.oafinfo
else
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-editor-control.gnorba
endif

uidir = $(datadir)/gnome/ui
ui_DATA = html-editor-control.xml

# The Test program

noinst_PROGRAMS =				\
	test-html-editor-control

test_html_editor_control_SOURCES =		\
	$(IDL_GENERATED)			\
	resolver.c				\
	resolver.h				\
	test-html-editor-control.c

# Extra stuff to distribute.

EXTRA_DIST =					\
	$(IDLS)					\
	html-editor-control.gnorba		\
	html-editor-control.oafinfo		\
	ChangeLog				\
	$(ui_DATA)
