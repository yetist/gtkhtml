INCLUDES = -I../src -I$(includedir) $(GNOME_INCLUDEDIR) -DGNOMELOCALEDIR=\"$(datadir)/locale\"
CFLAGS +=  $(GCONF_CFLAGS)

bin_PROGRAMS = gtkhtml-properties-capplet
gtkhtml_properties_capplet_SOURCES = \
	main.c

gtkhtml_properties_capplet_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOME_CAPPLET_LIBS) \
	$(GCONF_LIBS) -lgconf-gtk ../src/gtkhtml-properties.o
	

EXTRA_DIST = gtkhtml-properties.desktop

sysdir = $(datadir)/control-center/Documents/
sys_DATA = gtkhtml-properties.desktop

deskdir = $(datadir)/gnome/apps/Settings/Documents/
desk_DATA = gtkhtml-properties.desktop
