gladedir = $(datadir)/gtkhtml
glade_DATA = gtkhtml-capplet.glade

INCLUDES =					\
	-I$(top_srcdir)/src			\
	-I$(includedir)				\
	$(GNOME_INCLUDEDIR)			\
	$(EXTRA_GNOME_CFLAGS)			\
	-DGNOMELOCALEDIR=\"$(datadir)/locale\"	\
	-DGLADE_DATADIR=\"$(gladedir)\"

CFLAGS +=  $(GCONF_CFLAGS)

bin_PROGRAMS = gtkhtml-properties-capplet
gtkhtml_properties_capplet_SOURCES = 		\
	gnome-bindings-prop.h 			\
	gnome-bindings-prop.c			\
	main.c

LDADD = \
	$(GNOME_LIBDIR) 		\
	$(EXTRA_GNOME_LIBS)		\
	$(GNOME_CAPPLET_LIBS) 		\
	$(GCONF_LIBS) 			\
	$(top_builddir)/src/libgtkhtml.la

EXTRA_DIST = \
	gtkhtml-properties.desktop	\
	gtkhtml-capplet.glade

sysdir = $(datadir)/control-center/Documents/
sys_DATA = gtkhtml-properties.desktop

deskdir = $(datadir)/gnome/apps/Settings/Documents/
desk_DATA = gtkhtml-properties.desktop
