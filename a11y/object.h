/*  This file is part of the GtkHTML library.
 *
 *  Copyright 2002 Ximian, Inc.
 *
 *  Author: Radek Doulik
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef __GTK_HTML_A11Y_OBJECT_H__
#define __GTK_HTML_A11Y_OBJECT_H__

#include <gtk/gtk.h>
#include <gtk/gtk-a11y.h>

G_BEGIN_DECLS

#define G_TYPE_GTK_HTML_A11Y            (gtk_html_a11y_get_type ())
#define G_IS_GTK_HTML_A11Y(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_GTK_HTML_A11Y))

G_DECLARE_FINAL_TYPE (GtkHTMLA11Y, gtk_html_a11y, GTK, HTML_A11Y, GtkContainerAccessible)

#define GTK_HTML_ID "gtk-html-widget"
#define GTK_HTML_A11Y_GTKHTML(o) GTK_HTML (g_object_get_data (G_OBJECT (o), GTK_HTML_ID))
#define GTK_HTML_A11Y_GTKHTML_POINTER(o) g_object_get_data (G_OBJECT (o), GTK_HTML_ID)

AtkObject * gtk_html_a11y_new (GtkWidget *widget);

G_END_DECLS

#endif
