
bin_PROGRAMS = ebrowser

INCLUDES = \
	-I$(top_srcdir)/src	\
	$(GNOME_INCLUDEDIR)

CFLAGS += $(EXTRA_GNOME_CFLAGS)

EBROWSER_ENGINE_FILES = \
	ebrowser-widget.c ebrowser-widget.h \
	eloader.c eloader.h \
	eloader-http.c eloader-http.h \
	eloader-file.c eloader-file.h \
	e-http-client.c e-http-client.h \
	e-cache.c e-cache.h

ebrowser_SOURCES = \
	$(EBROWSER_ENGINE_FILES) \
	ebrowser.c ebrowser.h

LDADD = \
	$(top_builddir)/src/libgtkhtml.la \
	$(EXTRA_GNOME_LIBS) \
	$(GHTTP_LIBS)


noinst_PROGRAMS = test test-ebrowser

test_SOURCES = \
	$(EBROWSER_ENGINE_FILES) \
	test.c test.h

test_ebrowser_SOURCES = \
	$(EBROWSER_ENGINE_FILES) \
	test-ebrowser.c test-ebrowser.h

oafdir = $(datadir)/oaf

OAF_FILES = ebrowser.oafinfo

oaf_DATA = $(OAF_FILES)

EXTRA_DIST = \
	$(OAF_FILES)	\
	ChangeLog



