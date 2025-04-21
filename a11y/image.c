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

#include <config.h>
#include <atk/atkimage.h>

#include "htmlimage.h"

#include "html.h"
#include "image.h"

#include <glib/gi18n-lib.h>

struct _HTMLA11YImage {
	HTMLA11Y html_a11y_object;
};

static void atk_image_interface_init      (AtkImageIface *iface);

static const gchar * html_a11y_image_get_name (AtkObject *accessible);

static void html_a11y_image_get_image_position (AtkImage *image, gint *x, gint *y, AtkCoordType coord_type);
static void html_a11y_image_get_image_size (AtkImage *image, gint *width, gint *height);
static const gchar *html_a11y_image_get_image_description (AtkImage *image);

G_DEFINE_TYPE_EXTENDED (HTMLA11YImage,
                       html_a11y_image,
                       G_TYPE_HTML_A11Y,
                       0,
                       G_IMPLEMENT_INTERFACE (ATK_TYPE_IMAGE,
                         atk_image_interface_init));

static void
atk_image_interface_init (AtkImageIface *iface)
{
	g_return_if_fail (iface != NULL);

	iface->get_image_position = html_a11y_image_get_image_position;
	iface->get_image_size = html_a11y_image_get_image_size;
	iface->get_image_description = html_a11y_image_get_image_description;
}

static void
html_a11y_image_finalize (GObject *obj)
{
	G_OBJECT_CLASS (html_a11y_image_parent_class)->finalize (obj);
}

static void
html_a11y_image_initialize (AtkObject *obj,
                            gpointer data)
{
	/* printf ("html_a11y_image_initialize\n"); */

	if (ATK_OBJECT_CLASS (html_a11y_image_parent_class)->initialize)
		ATK_OBJECT_CLASS (html_a11y_image_parent_class)->initialize (obj, data);
}

static void
html_a11y_image_class_init (HTMLA11YImageClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	html_a11y_image_parent_class = g_type_class_peek_parent (klass);

	atk_class->get_name = html_a11y_image_get_name;
	atk_class->initialize = html_a11y_image_initialize;
	gobject_class->finalize = html_a11y_image_finalize;
}

static void
html_a11y_image_init (HTMLA11YImage *a11y_image)
{
}

AtkObject *
html_a11y_image_new (HTMLObject *html_obj)
{
	GObject *object;
	AtkObject *accessible;

	g_return_val_if_fail (HTML_IS_IMAGE (html_obj), NULL);

	object = g_object_new (G_TYPE_HTML_A11Y_IMAGE, NULL);

	accessible = ATK_OBJECT (object);
	atk_object_initialize (accessible, html_obj);

	accessible->role = ATK_ROLE_IMAGE;

	/* printf ("created new html accessible image object\n"); */

	return accessible;
}

static const gchar *
html_a11y_image_get_name (AtkObject *accessible)
{
	HTMLImage *img = HTML_IMAGE (HTML_A11Y_HTML (accessible));
	gchar *name;

	if (accessible->name)
		return accessible->name;

	if (img->alt) {
		name = g_strdup_printf (_("URL is %s, Alternative Text is %s"), img->image_ptr->url, img->alt);
	} else
		name = g_strdup_printf (_("URL is %s"), img->image_ptr->url);

	atk_object_set_name (accessible, name);
	g_free (name);

	return accessible->name;
}

/*
 * AtkImage interface
 */

static void
html_a11y_image_get_image_position (AtkImage *image,
                                    gint *x,
                                    gint *y,
                                    AtkCoordType coord_type)
{
	HTMLImage *img = HTML_IMAGE (HTML_A11Y_HTML (image));

	atk_component_get_position (ATK_COMPONENT (image), x, y, coord_type);

	*x += img->hspace + img->border;
	*y += img->vspace + img->border;
}

static void
html_a11y_image_get_image_size (AtkImage *image,
                                gint *width,
                                gint *height)
{
	HTMLImage *img = HTML_IMAGE (HTML_A11Y_HTML (image));

	atk_component_get_size (ATK_COMPONENT (image), width, height);

	*width  -= 2*(img->hspace + img->border);
	*height -= 2*(img->vspace + img->border);
}

static const gchar *
html_a11y_image_get_image_description (AtkImage *image)
{
	return  html_a11y_image_get_name (ATK_OBJECT (image));
}
