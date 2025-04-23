/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-swatch.c
 *
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2025 yetist <yetist@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <glib/gi18n-lib.h>
#include "gtkhtml-color-swatch.h"

enum {
	PROP_0,
	PROP_COLOR,
	PROP_SHADOW_TYPE
};

struct _GtkhtmlColorSwatch {
	GtkBin     bin;
	GtkWidget *drawing_area;
	GtkWidget *frame;
};

G_DEFINE_TYPE (GtkhtmlColorSwatch, gtkhtml_color_swatch, GTK_TYPE_BIN);

static gboolean
color_swatch_draw_cb (GtkWidget *drawing_area,
                      cairo_t *cr)
{
	GtkStyleContext *style_context;
	GdkRGBA rgba;
	GdkRectangle rect;

	style_context = gtk_widget_get_style_context (drawing_area);
	if (!style_context)
		return FALSE;

	gtk_style_context_get_background_color (style_context, GTK_STATE_FLAG_NORMAL, &rgba);

	gdk_cairo_get_clip_rectangle (cr, &rect);
	gdk_cairo_set_source_rgba (cr, &rgba);
	gdk_cairo_rectangle (cr, &rect);
	cairo_fill (cr);

	return FALSE;
}

static void
color_swatch_set_property (GObject *object,
                           guint property_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_COLOR:
			gtkhtml_color_swatch_set_color (
				GTKHTML_COLOR_SWATCH (object),
				g_value_get_boxed (value));
			return;

		case PROP_SHADOW_TYPE:
			gtkhtml_color_swatch_set_shadow_type (
				GTKHTML_COLOR_SWATCH (object),
				g_value_get_enum (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
color_swatch_get_property (GObject *object,
                           guint property_id,
                           GValue *value,
                           GParamSpec *pspec)
{
	GdkColor color;

	switch (property_id) {
		case PROP_COLOR:
			gtkhtml_color_swatch_get_color (
				GTKHTML_COLOR_SWATCH (object), &color);
			g_value_set_boxed (value, &color);
			return;

		case PROP_SHADOW_TYPE:
			g_value_set_enum (
				value, gtkhtml_color_swatch_get_shadow_type (
				GTKHTML_COLOR_SWATCH (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
color_swatch_get_preferred_width (GtkWidget *widget,
                                  gint *minimum_width,
                                  gint *natural_width)
{
	GtkhtmlColorSwatch *swatch;

	swatch = GTKHTML_COLOR_SWATCH (widget);

	gtk_widget_get_preferred_width (
		swatch->frame, minimum_width, natural_width);
}

static void
color_swatch_get_preferred_height (GtkWidget *widget,
                                   gint *minimum_height,
                                   gint *natural_height)
{
	GtkhtmlColorSwatch *swatch;

	swatch = GTKHTML_COLOR_SWATCH (widget);

	gtk_widget_get_preferred_height (
		swatch->frame, minimum_height, natural_height);
}

static void
color_swatch_size_allocate (GtkWidget *widget,
                            GtkAllocation *allocation)
{
	GtkhtmlColorSwatch *swatch;

	swatch = GTKHTML_COLOR_SWATCH (widget);

	gtk_widget_set_allocation (widget, allocation);
	gtk_widget_size_allocate (swatch->frame, allocation);
}

static void
gtkhtml_color_swatch_class_init (GtkhtmlColorSwatchClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->set_property = color_swatch_set_property;
	object_class->get_property = color_swatch_get_property;

	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->get_preferred_width = color_swatch_get_preferred_width;
	widget_class->get_preferred_height = color_swatch_get_preferred_height;
	widget_class->size_allocate = color_swatch_size_allocate;

	g_object_class_install_property (
		object_class,
		PROP_COLOR,
		g_param_spec_boxed (
			"color",
			"Color",
			"The current color",
			GDK_TYPE_COLOR,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHADOW_TYPE,
		g_param_spec_enum (
			"shadow-type",
			"Frame Shadow",
			"Appearance of the frame border",
			GTK_TYPE_SHADOW_TYPE,
			GTK_SHADOW_ETCHED_IN,
			G_PARAM_READWRITE));
}

static void
gtkhtml_color_swatch_init (GtkhtmlColorSwatch *swatch)
{
	swatch->frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (swatch), swatch->frame);
	gtk_widget_show (swatch->frame);

	swatch->drawing_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request (swatch->drawing_area, 16, 16);
	gtk_container_add (GTK_CONTAINER (swatch->frame), swatch->drawing_area);
	gtk_widget_show (swatch->drawing_area);

	g_signal_connect (
		swatch->drawing_area, "draw",
		G_CALLBACK (color_swatch_draw_cb), swatch);
}

GtkWidget*
gtkhtml_color_swatch_new (void)
{
	GtkhtmlColorSwatch *self;
	self = g_object_new (GTKHTML_TYPE_COLOR_SWATCH, NULL);
	return GTK_WIDGET(self);
}

void
gtkhtml_color_swatch_get_color (GtkhtmlColorSwatch *swatch,
                                GdkColor *color)
{
	GtkStyleContext *style_context;
	GtkWidget *drawing_area;
	GdkRGBA rgba;

	g_return_if_fail (GTKHTML_IS_COLOR_SWATCH (swatch));
	g_return_if_fail (color != NULL);

	drawing_area = swatch->drawing_area;
	style_context = gtk_widget_get_style_context (drawing_area);
	gtk_style_context_get_background_color (style_context, GTK_STATE_FLAG_NORMAL, &rgba);

	color->red   = rgba.red * 65535;
	color->green = rgba.green * 65535;
	color->blue  = rgba.blue * 65535;
}

void
gtkhtml_color_swatch_set_color (GtkhtmlColorSwatch *swatch,
                                const GdkColor *color)
{
	GtkWidget *drawing_area;

	g_return_if_fail (GTKHTML_IS_COLOR_SWATCH (swatch));

	drawing_area = swatch->drawing_area;
	gtk_widget_modify_bg (drawing_area, GTK_STATE_NORMAL, color);

	g_object_notify (G_OBJECT (swatch), "color");
}

GtkShadowType
gtkhtml_color_swatch_get_shadow_type (GtkhtmlColorSwatch *swatch)
{
	GtkFrame *frame;

	g_return_val_if_fail (GTKHTML_IS_COLOR_SWATCH (swatch), 0);

	frame = GTK_FRAME (swatch->frame);

	return gtk_frame_get_shadow_type (frame);
}

void
gtkhtml_color_swatch_set_shadow_type (GtkhtmlColorSwatch *swatch,
                                      GtkShadowType shadow_type)
{
	GtkFrame *frame;

	g_return_if_fail (GTKHTML_IS_COLOR_SWATCH (swatch));

	frame = GTK_FRAME (swatch->frame);
	gtk_frame_set_shadow_type (frame, shadow_type);

	g_object_notify (G_OBJECT (swatch), "shadow-type");
}
