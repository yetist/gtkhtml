# Basic setup

INCLUDES =						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)/src				\
	-I$(srcdir)					\
	-I.						\
        $(GNOME_INCLUDEDIR)				\
	$(EXTRA_GNOME_CFLAGS)				\
	$(GTK_CFLAGS)


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
	editor-control-factory.c		\
	editor-control-factory.h		\
	image.c					\
	image.h					\
	main.c					\
	menubar.c				\
	menubar.h				\
	persist-stream-impl.c			\
	persist-stream-impl.h			\
	replace.c				\
	replace.h				\
	search.c				\
	search.h				\
	toolbar.c				\
	toolbar.h

gnorbadir = $(sysconfdir)/CORBA/servers
gnorba_DATA = html-editor-control.gnorba

# The Test program

noinst_PROGRAMS =				\
	test-html-editor-control

test_html_editor_control_SOURCES =		\
	test-html-editor-control.c

# Extra stuff to distribute.

EXTRA_DIST =					\
	$(gnorba_DATA)				\
	ChangeLog
