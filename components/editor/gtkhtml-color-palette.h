/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-palette.h
 *
 * Copyright (C) 2008 Novell, Inc.
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTKHTML_TYPE_COLOR_PALETTE              (gtkhtml_color_palette_get_type ())

G_DECLARE_FINAL_TYPE (GtkhtmlColorPalette, gtkhtml_color_palette, GTKHTML, COLOR_PALETTE, GObject)

GtkhtmlColorPalette* gtkhtml_color_palette_new	       (void);
void		     gtkhtml_color_palette_add_color   (GtkhtmlColorPalette *palette, const GdkColor *color);
GSList *	     gtkhtml_color_palette_list_colors (GtkhtmlColorPalette *palette);

G_END_DECLS
