# Basic setup

INCLUDES =						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)/src				\
	-I$(srcdir)					\
	-I.						\
        $(GNOME_INCLUDEDIR)				\
	$(EXTRA_GNOME_CFLAGS)				\
	$(GTK_CFLAGS)

CFLAGS +=  $(GCONF_CFLAGS)

LDADD =						\
	$(top_builddir)/src/libgtkhtml.la	\
	$(EXTRA_GNOME_LIBS)			\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBS)				\
	$(GCONF_LIBS)
#	-lccmalloc -ldl

# The Editor Control

bin_PROGRAMS =					\
	html-editor-control

html_editor_control_SOURCES =			\
	body.h					\
	body.c					\
	control-data.c				\
	control-data.h				\
	dialog.c				\
	dialog.h				\
	editor-control-factory.c		\
	editor-control-factory.h		\
	gtk-combo-box.c				\
	gtk-combo-box.h				\
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
	rule.c					\
	rule.h					\
	search.c				\
	search.h				\
	text.c					\
	text.h					\
	toolbar.c				\
	toolbar.h				\
	utils.c					\
	utils.h

if USING_OAF
oafdir = $(datadir)/oaf
oaf_DATA = html-editor-control.oafinfo
else
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-editor-control.gnorba
endif

# The Test program

noinst_PROGRAMS =				\
	test-html-editor-control

test_html_editor_control_SOURCES =		\
	test-html-editor-control.c

# Extra stuff to distribute.

EXTRA_DIST =					\
	html-editor-control.gnorba		\
	html-editor-control.oafinfo		\
	ChangeLog
