/*
 * Copyright (C) 2025 Xiaotian Wu <yetist@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <gtkhtml/gtkhtml-version.h>

/**
 * SECTION: gtkhtml-version
 * @Short_description: Variables and functions to check the GtkHTML version
 * @Title: Version Information
 * @include: gtkhtml/gtkhtml-version.h
 * @stability: Stable
 *
 * GtkHTML provides version information, primarily useful in
 * configure checks for builds that have a configure script.
 * Applications will not typically use the features described here.
 */

const guint gtk_html_major_version = GTK_HTML_MAJOR_VERSION;
const guint gtk_html_minor_version = GTK_HTML_MINOR_VERSION;
const guint gtk_html_micro_version = GTK_HTML_MICRO_VERSION;

/**
 * gtk_html_check_version:
 * @required_major: the required major version.
 * @required_minor: the required minor version.
 * @required_micro: the required micro version.
 *
 * Checks that the GtkHTML library in use is compatible with the
 * given version. Generally you would pass in the constants
 * #GTK_HTML_MAJOR_VERSION, #GTK_HTML_MINOR_VERSION, #GTK_HTML_MICRO_VERSION
 * as the three arguments to this function; that produces
 * a check that the library in use is compatible with
 * the version of GtkHTML the application or module was compiled
 * against.
 *
 * Compatibility is defined by two things: first the version
 * of the running library is newer than the version
 * @required_major.required_minor.@required_micro. Second
 * the running library must be binary compatible with the
 * version @required_major.required_minor.@required_micro
 * (same major version.)
 *
 * Return value: %NULL if the GtkHTML library is compatible with the
 *   given version, or a string describing the version mismatch.
 *   The returned string is owned by GtkHTML and must not be modified
 *   or freed.
 *
 * Since: 4.12.0
 **/
const gchar * gtk_html_check_version (guint required_major,
				      guint required_minor,
				      guint required_micro)
{
    gint gtk_html_effective_micro = 100 * GTK_HTML_MINOR_VERSION + GTK_HTML_MICRO_VERSION;
    gint required_effective_micro = 100 * required_minor + required_micro;

    if (required_major > GTK_HTML_MAJOR_VERSION)
        return "GtkHTML version too old (major mismatch)";
    if (required_major < GTK_HTML_MAJOR_VERSION)
        return "GtkHTML version too new (major mismatch)";
    if (required_effective_micro < gtk_html_effective_micro - GTK_HTML_BINARY_AGE)
        return "GtkHTML version too new (micro mismatch)";
    if (required_effective_micro > gtk_html_effective_micro)
        return "GtkHTML version too old (micro mismatch)";
    return NULL;
}
