## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=gtkhtml

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# The directory containing the source code (if it contains documentation).

DOC_SOURCE_DIR=$(top_srcdir)/gtkhtml

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED" 
SCAN_OPTIONS=--deprecated-guards="GLIB_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--xml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
HFILE_GLOB=$(top_srcdir)/gtkhtml/*.h
CFILE_GLOB=$(top_srcdir)/gtkhtml/*.c

# Extra header to include when scanning, which are not under DOC_SOURCE_DIR
# # e.g. EXTRA_HFILES=$(top_srcdir}/contrib/extra.h
EXTRA_HFILES=

IGNORE_HFILES=

# Images to copy into HTML directory.
HTML_IMAGES=

# Extra files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.xml building.xml changes-2.0.xml
content_files=

# Files where gtk-doc abbrevations (#GtkWidget) are expanded
# e.g. expand_content_files=running.xml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
#
GTKDOC_CFLAGS = \
		$(GTKHTML_CFLAGS)	\
		-I$(top_srcdir)/gtkhtml                        \
		-I$(top_builddir)/gtkhtml

GTKDOC_LIBS = \
        $(top_builddir)/gtkhtml/libgtkhtml-4.0.la \
	$(GAIL_LIBS)				\
	$(REGEX_LIBS)				\
	$(LIBM)	\
	$(GTKHTML_LIBS)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = \
  DOC_MODULE=$(DOC_MODULE) DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
  SRCDIR=$(abs_srcdir) BUILDDIR=$(abs_builddir)
TESTS = $(GTKDOC_CHECK)
endif

-include $(top_srcdir)/git.mk
