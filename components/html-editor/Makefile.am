# Basic setup

INCLUDES =						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)/src				\
	-I$(srcdir)					\
	-I.						\
        $(GNOME_INCLUDEDIR)				\
	$(GTK_CFLAGS)


LDADD =						\
	$(top_builddir)/src/libgtkhtml.la	\
	$(GNOMEUI_LIBS)				\
	$(GNOME_LIBS)				\
	$(EXTRA_GNOME_LIBS)

# The Editor Control

bin_PROGRAMS =					\
	html-editor-control

html_editor_control_SOURCES =			\
	editor-control-factory.c		\
	editor-control-factory.h		\
	main.c					\
	menubar.c				\
	menubar.h				\
	persist-stream-impl.c			\
	persist-stream-impl.h			\
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
