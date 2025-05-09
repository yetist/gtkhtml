dnl Initialization
AC_PREREQ(2.62)

m4_define([gtk_html_major_version], [4])
m4_define([gtk_html_minor_version], [12])
m4_define([gtk_html_micro_version], [0])
m4_define([gtk_html_interface_age], [0])
m4_define([gtk_html_binary_age], [m4_eval(100 * gtk_html_minor_version + gtk_html_micro_version)])
m4_define([gtk_html_version], [gtk_html_major_version.gtk_html_minor_version.gtk_html_micro_version])

m4_define([gtk_html_lt_current],
	  [m4_eval(100 * gtk_html_minor_version + gtk_html_micro_version - gtk_html_interface_age)])
m4_define([gtk_html_lt_revision], [gtk_html_interface_age])
m4_define([gtk_html_lt_age], [m4_eval(gtk_html_binary_age - gtk_html_interface_age)])

AC_INIT(gtkhtml, gtk_html_version, [https://github.com/yetist/gtkhtml/issues/new])
AC_COPYRIGHT([Copyright (C) 2025 yetist@gmail.com])
AM_INIT_AUTOMAKE([gnu 1.9 dist-xz no-dist-gzip -Wno-portability])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(README)

LT_VERSION_INFO="gtk_html_lt_current:gtk_html_lt_revision:gtk_html_lt_age"
LT_CURRENT_MINUS_AGE=m4_eval(gtk_html_lt_current - gtk_html_lt_age)
AC_SUBST(LT_VERSION_INFO)
AC_SUBST(LT_CURRENT_MINUS_AGE)

GTK_HTML_VERSION=gtk_html_version
GTK_HTML_BINARY_AGE=gtk_html_binary_age
AC_SUBST(GTK_HTML_VERSION)
AC_SUBST(GTK_HTML_BINARY_AGE)

GTK_HTML_MAJOR_VERSION=gtk_html_major_version
GTK_HTML_MINOR_VERSION=gtk_html_minor_version
GTK_HTML_MICRO_VERSION=gtk_html_micro_version

AC_SUBST(GTK_HTML_MAJOR_VERSION)
AC_SUBST(GTK_HTML_MINOR_VERSION)
AC_SUBST(GTK_HTML_MICRO_VERSION)

AC_DEFINE(GTK_HTML_MAJOR_VERSION, [gtk_html_major_version],
	  [Define to the GTK_HTML major version])
AC_DEFINE(GTK_HTML_MINOR_VERSION, [gtk_html_minor_version],
	  [Define to the GTK_HTML minor version])
AC_DEFINE(GTK_HTML_MICRO_VERSION, [gtk_html_micro_version],
	  [Define to the GTK_HTML micro version])
AC_DEFINE(GTK_HTML_BINARY_AGE, [gtk_html_binary_age],
	  [Define to the GTK_HTML binary age])

PACKAGE=gtkhtml

dnl Required Package Versions
m4_define([gtk_minimum_version], [3.2.0])
m4_define([enchant_minimum_version], [2.2.0])
m4_define([cairo_minimum_version], [1.10.0])

GTKHTML_API_VERSION=4.0
EDITOR_API_VERSION=4.0

AC_DEFINE_UNQUOTED(GTKHTML_API_VERSION, "${GTKHTML_API_VERSION}", [GtkHTML API Version])
AC_SUBST(GTKHTML_API_VERSION)

AC_DEFINE_UNQUOTED(EDITOR_API_VERSION, "${EDITOR_API_VERSION}", [Editor API Version])
AC_SUBST(EDITOR_API_VERSION)

GTKHTML_RELEASE_STRING=${PACKAGE}-${GTKHTML_API_VERSION}
AC_DEFINE_UNQUOTED(GTKHTML_RELEASE_STRING, "${GTKHTML_RELEASE_STRING}", [GtkHTML Release String])
AC_SUBST(GTKHTML_RELEASE_STRING)

GETTEXT_PACKAGE=${GTKHTML_RELEASE_STRING}
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Package name for gettext])

I18N_LIB_CFLAGS="-DGNOME_EXPLICIT_TRANSLATION_DOMAIN=\\\"${GTKHTML_RELEASE_STRING}\\\""
AC_SUBST(I18N_LIB_CFLAGS)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE([enable])

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC

AC_PROG_INSTALL
AC_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.36.3])
AM_GLIB_GNU_GETTEXT

GTK_DOC_CHECK([1.0],[--flavour=no-tmpl])

AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
dnl **********************************
dnl Compiler Warning Flags
dnl **********************************
AS_COMPILER_FLAGS(WARNING_FLAGS,
	"-Wall -Wextra
	-Wno-missing-field-initializers
	-Wno-sign-compare
	-Wno-unused-parameter
	-Wno-deprecated-declarations
	-Wdeclaration-after-statement
	-Werror-implicit-function-declaration
	-Wformat-nonliteral -Wformat-security -Winit-self
	-Wmissing-declarations -Wmissing-include-dirs
	-Wmissing-noreturn -Wnested-externs -Wpointer-arith
	-Wredundant-decls -Wundef -Wwrite-strings")
AC_SUBST(WARNING_FLAGS)

dnl Other useful compiler warnings for test builds only.
dnl These may produce warnings we have no control over,
dnl or false positives we don't always want to see.
dnl
dnl	-Wmissing-format-attribute
dnl	-Wshadow
dnl	-Wstrict-aliasing=2

dnl XXX This really belongs in AM_CPPFLAGS.
CFLAGS="$CFLAGS $WARNING_FLAGS"

dnl *********************
dnl Necessary programs
dnl *********************
AC_PROG_CC
AC_PROG_CXX
AC_C_INLINE
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_TYPE_SIZE_T

AC_CHECK_FUNCS([memchr memmove memset regcomp setlocale strchr strcspn strrchr strstr strtol strtoull])
AC_CHECK_HEADERS([fcntl.h libintl.h])

dnl GCC 4.4 got more aggressive in its aliasing optimizations, changing
dnl behavior that -- according to the C99 standard -- is supposed to be
dnl undefined.  We may still have aliasing abuses lying around that rely
dnl on GCC's previous "undefined" behavior, so disable strict-aliasing
dnl optimization until we can find and fix all the abuses.
dnl (AC_PROG_CC must run first to set the GCC variable.)
dnl XXX This really belongs in AM_CFLAGS.
if test "x${GCC}" = "xyes"; then
	CFLAGS="$CFLAGS -fno-strict-aliasing"
fi

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

dnl ********
dnl Win32
dnl ********
AC_MSG_CHECKING([for Win32])
case "$host" in
*-mingw*)
    os_win32=yes
    SOEXT='.dll'
    ;;
*)  os_win32=no
    SOEXT='.so'
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, [test $os_win32 = yes])
AC_SUBST(SOEXT)

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

AC_CHECK_LIBM
AC_SUBST(LIBM)

dnl **********************************
dnl GAIL modules
dnl **********************************
GAIL_MODULES="gail-3.0 >= gtk_minimum_version"
PKG_CHECK_MODULES(GAIL, $GAIL_MODULES)
AC_SUBST(GAIL_CFLAGS)
AC_SUBST(GAIL_LIBS)
AC_SUBST(GAIL_MODULES)

dnl **********************************
dnl GTKHTML modules
dnl **********************************
GTKHTML_MODULES="gtk+-3.0 >= gtk_minimum_version enchant-2 >= enchant_minimum_version iso-codes cairo >= cairo_minimum_version"
PKG_CHECK_MODULES(GTKHTML, $GTKHTML_MODULES)
AC_SUBST(GTKHTML_CFLAGS)
AC_SUBST(GTKHTML_LIBS)
AC_SUBST(GTKHTML_MODULES)

dnl **********************************
dnl Glade catalog files
dnl **********************************
AC_ARG_WITH(glade-catalog,
	AC_HELP_STRING([--with-glade-catalog],
	[install the catalog files for Glade 3]
	[(for maintainers only) @<:@default=no@:>@]),
	[glade_catalog="$withval"], [glade_catalog="no"])
if test "x$glade_catalog" = "xyes"; then
	PKG_CHECK_MODULES(GLADEUI, gladeui-2.0)
fi
AM_CONDITIONAL(GLADE_CATALOG, test x$glade_catalog = xyes)

dnl **********************************
dnl soup for testgtkhtml
dnl **********************************
HAVE_NEWSOUP="no"
have_newsoup="no"
PKG_CHECK_MODULES(SOUP, libsoup-2.4  >= 2.26.0,
	[	HAVE_NEWSOUP="yes"
		have_soup="yes"
	],
	[	PKG_CHECK_MODULES(SOUP, libsoup-2.4,
		have_soup="yes",
		have_soup="no")
	])
AC_SUBST(HAVE_NEWSOUP)
AM_CONDITIONAL(HAVE_SOUP, test x$have_soup != xno)
AM_CONDITIONAL(HAVE_NEWSOUP, test x$HAVE_NEWSOUP != xno)

dnl **************************************************
dnl iso-codes
dnl **************************************************
AC_MSG_CHECKING([for iso-codes package])
have_iso_codes=no
if $PKG_CONFIG --exists iso-codes; then
	if $PKG_CONFIG iso-codes --atleast-version=0.49; then
		have_iso_codes=yes
		AC_MSG_RESULT([$have_iso_codes])
	else
		AC_MSG_WARN([iso-codes detected, but version 0.49 or later is required due to licensing])
	fi
else
	AC_MSG_RESULT([$have_iso_codes])
fi

if test "x$have_iso_codes" = "xyes"; then
	AC_MSG_CHECKING([whether iso-codes has iso-639 and iso-3166 domains])
	if $PKG_CONFIG --variable=domains iso-codes | grep 639 >/dev/null 2>&1 && \
		$PKG_CONFIG --variable=domains iso-codes | grep 3166 >/dev/null 2>&1 ; then
			result=yes
	else
		result=no
		have_iso_codes=no
	fi
	AC_MSG_RESULT([$result])
fi

if test "x$have_iso_codes" = "xyes"; then
	AC_DEFINE_UNQUOTED([ISO_CODES_PREFIX],
		["`$PKG_CONFIG --variable=prefix iso-codes`"],
		[ISO codes prefix])
	AC_DEFINE([HAVE_ISO_CODES], [1],
		[Define if you have the iso-codes package])
fi

dnl **************************************************
dnl XF86 multimedia keys support
dnl **************************************************
AC_CHECK_HEADER([X11/XF86keysym.h],
	[AC_DEFINE([HAVE_XFREE], 1, [Have <X11/XF86keysym.h>])],,
	[[	#if HAVE_XFREE
		#include <X11/XF86keysym.h>
		#endif
	]])

dnl **************************************************
dnl regex library
dnl **************************************************
AC_CHECK_FUNCS(regexec,,[AC_CHECK_LIB(regex,regexec,[REGEX_LIBS=-lregex],[AC_MSG_ERROR([No regex library found])])])
AC_SUBST(REGEX_LIBS)

dnl ******************************
dnl shlib factory
dnl ******************************
AC_ARG_ENABLE([shlib-factory],
		AS_HELP_STRING([--enable-shlib-factory],
		[build editor factory as shared library @<:@default=yes@:>@]),
		[enable_shlib_factory="$enableval"],
		[enable_shlib_factory="yes"])

if test "x$enable_shlib_factory" = "xyes"; then
	GNOME_GTKHTML_EDITOR_SHLIB=1
	GNOME_GTKHTML_EDITOR_TYPE="shlib"
	AC_DEFINE(GNOME_GTKHTML_EDITOR_SHLIB,, [Editor shlib])
else
	GNOME_GTKHTML_EDITOR_TYPE="exe"
fi

AC_SUBST(GNOME_GTKHTML_EDITOR_TYPE)
AM_CONDITIONAL(EDITOR_SHLIB, test x$enable_shlib_factory = xyes)

AC_CONFIG_FILES([
Makefile
a11y/Makefile
art/Makefile
icons/Makefile
components/Makefile
components/editor/Makefile
components/editor/gtkhtml-editor.pc
gtkhtml/gtkhtml-version.h
gtkhtml/libgtkhtml.pc
gtkhtml/Makefile
po/Makefile.in
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
])

AC_OUTPUT
AC_MSG_RESULT([
Configuration:

  Version                 : ${VERSION}
  Install prefix          : ${prefix}
  Cookies support in soup : ${HAVE_NEWSOUP}
  Editor component type   : ${GNOME_GTKHTML_EDITOR_TYPE}
  Build document          : ${enable_gtk_doc}
])
