INCLUDES = -I../src -I$(includedir) $(GNOME_INCLUDEDIR) -DGNOMELOCALEDIR=\"$(datadir)/locale\"
CFLAGS +=  $(GCONF_CFLAGS)

bin_PROGRAMS = gtkhtml-properties-capplet
gtkhtml_properties_capplet_SOURCES = \
	gnome-bindings-prop.c	\
	main.c

gtkhtml_properties_capplet_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOME_CAPPLET_LIBS) \
	$(GCONF_LIBS) ../src/gtkhtml-properties.o

#	../src/libgtkhtml.la \
#	$(GNOMEUI_LIBS)				\
#	$(GNOME_LIBDIR)				\
#	$(EXTRA_GNOME_LIBS)

EXTRA_DIST = gtkhtml-properties.desktop

sysdir = $(datadir)/control-center/Documents/
sys_DATA = gtkhtml-properties.desktop

deskdir = $(datadir)/gnome/apps/Settings/Documents/
desk_DATA = gtkhtml-properties.desktop
