# Basic setup

INCLUDES =						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)/src				\
	-I$(srcdir)					\
	-I.						\
        $(GNOME_INCLUDEDIR)				\
	$(EXTRA_GNOME_CFLAGS)				\
	$(GTK_CFLAGS)

# LDFLAGS += -static
LDADD =						\
	$(top_builddir)/src/libgtkhtml.la	\
	$(EXTRA_GNOME_LIBS)			\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBS)

# The Editor Control

bin_PROGRAMS =					\
	html-editor-control

html_editor_control_SOURCES =			\
	control-data.c				\
	control-data.h				\
	dialog.c				\
	dialog.h				\
	editor-control-factory.c		\
	editor-control-factory.h		\
	image.c					\
	image.h					\
	link.c					\
	link.h					\
	main.c					\
	menubar.c				\
	menubar.h				\
	persist-stream-impl.c			\
	persist-stream-impl.h			\
	popup.c					\
	popup.h					\
	replace.c				\
	replace.h				\
	search.c				\
	search.h				\
	toolbar.c				\
	toolbar.h

if USING_OAF
gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-editor-control.gnorba
else
oafdir = $(datadir)/oaf
oaf_DATA = html-editor-control.oafinfo
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
