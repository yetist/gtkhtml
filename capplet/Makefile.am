gladedir = $(datadir)/gtkhtml-@GTKHTML_API_VERSION@
glade_DATA = gtkhtml-capplet.glade

INCLUDES =					\
	-I$(top_srcdir)/src			\
	-I$(includedir)

CFLAGS +=  					\
	$(CAPPLET_CFLAGS)			\
	-DGTK_DISABLE_DEPRECATED		\
	-DGNOMELOCALEDIR=\"$(datadir)/locale\"	\
	-DGLADE_DATADIR=\"$(gladedir)\"

bin_PROGRAMS = gtkhtml-properties-capplet
gtkhtml_properties_capplet_SOURCES = 		\
	gnome-bindings-prop.h 			\
	gnome-bindings-prop.c			\
	main.c

LDADD = 					\
	$(CAPPLET_LIBS)				\
	$(top_builddir)/src/libgtkhtml-@GTKHTML_API_VERSION@.la	\
	$(top_builddir)/a11y/libgtkhtml-a11y-@GTKHTML_API_VERSION@.la

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/control-center-2.0/capplets
Desktop_in_files = gtkhtml-properties.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = 				\
	gtkhtml-capplet.glade		\
	$(Desktop_in_files)
