/*
 *  Copyright (C) 2000 Helix Code Inc.
 *
 *  Authors: Michael Zucchi <notzed@helixcode.com>
 *
 *  An embedded html widget.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_HTML_EMBEDDED         (gtk_html_embedded_get_type ())
#define GTK_IS_HTML_EMBEDDED(obj)      G_TYPE_CHECK_INSTANCE_TYPE (obj, GTK_TYPE_HTML_EMBEDDED)

G_DECLARE_DERIVABLE_TYPE (GtkHTMLEmbedded, gtk_html_embedded, GTK_HTML, EMBEDDED, GtkBin)

struct _GtkHTMLEmbeddedClass
{
  GtkBinClass     parent_class;

  void (*changed)    (GtkHTMLEmbedded *);
  void (*draw_gdk)   (GtkHTMLEmbedded *, cairo_t *, gint, gint);
  void (*draw_print) (GtkHTMLEmbedded *, GtkPrintContext *);
};

/* FIXME: There needs to be a way for embedded objects in forms to encode
 * themselves for a form */
GtkWidget*   gtk_html_embedded_new           (gchar *classid, gchar *name, gchar *type, gchar *data, gint width, gint height);
void         gtk_html_embedded_set_parameter (GtkHTMLEmbedded *ge, gchar *param, gchar *value);
gchar*       gtk_html_embedded_get_parameter (GtkHTMLEmbedded *ge, gchar *param);
void         gtk_html_embedded_set_descent   (GtkHTMLEmbedded *ge, gint descent);
// TODO: new api since XX
const gchar* gtk_html_embedded_get_classid   (GtkHTMLEmbedded *ge);
const gchar* gtk_html_embedded_get_name      (GtkHTMLEmbedded *ge);
gint         gtk_html_embedded_get_descent   (GtkHTMLEmbedded *ge);

G_END_DECLS
