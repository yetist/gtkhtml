/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries
   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   (C) 1997 Torben Weis (weis@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _HTMLOBJECT_H_
#define _HTMLOBJECT_H_

#include <glib.h>

#include "htmltype.h"
#include "htmlpainter.h"
#include "htmlfont.h"

#define HTML_OBJECT(x)		((HTMLObject *) (x))
#define HTML_OBJECT_CLASS(x)	((HTMLObjectClass *) (x))
#define HTML_OBJECT_TYPE(x)     (HTML_OBJECT (x)->klass->type)

typedef enum { HTMLNoFit, HTMLPartialFit, HTMLCompleteFit } HTMLFitType;

enum HTMLObjectFlags {
	HTML_OBJECT_FLAG_NONE = 0,
	HTML_OBJECT_FLAG_SEPARATOR = 1 << 0,
	HTML_OBJECT_FLAG_NEWLINE = 1 << 1,
	HTML_OBJECT_FLAG_SELECTED = 1 << 2,
	HTML_OBJECT_FLAG_ALLSELECTED = 1 << 3,
	HTML_OBJECT_FLAG_FIXEDWIDTH = 1 << 4,
	HTML_OBJECT_FLAG_ALIGNED = 1 << 5,
	HTML_OBJECT_FLAG_PRINTED = 1 << 6,
	HTML_OBJECT_FLAG_HIDDEN = 1 << 7
};

typedef enum { CNone, CLeft, CRight, CAll } ClearType;
typedef enum { Top, Bottom, VCenter, VNone } VAlignType;
typedef enum { Left, HCenter, Right, None } HAlignType;

typedef struct _HTMLObjectClass HTMLObjectClass;
typedef struct _HTMLAnchor HTMLAnchor;

struct _HTMLObject {
	HTMLObjectClass *klass;

	gint x, y;

	gint ascent, descent;
	HTMLFont *font;

	/* FIXME unsigned? */
	gshort width;
	gshort max_width;
	gshort percent;

	guchar flags;

	/* The absolute position of this object on the page */
	gint abs_x;
	gint abs_y;

	struct _HTMLObject *next;
};
typedef struct _HTMLObject HTMLObject;

struct _HTMLObjectClass {
	HTMLType type;

	void (*destroy) (HTMLObject *o);
	
        /* x & y are in object coordinates (e.g. the same coordinate system as
	   o->x and o->y) tx & ty are used to translated object coordinates
	   into painter coordinates */
	void (*draw) (HTMLObject *o, HTMLPainter *p,
		      gint x, gint y,
		      gint width, gint height,
		      gint tx, gint ty);

	HTMLFitType (*fit_line) (HTMLObject *o, gboolean start_of_line, 
				 gboolean first_run, gint width_left);

	void (*calc_size) (HTMLObject *o, HTMLObject *parent);

	void (*set_max_ascent) (HTMLObject *o, gint a);
	
	void (*set_max_descent) (HTMLObject *o, gint d);
	
	void (*set_max_width) (HTMLObject *o, gint max_width);
	
	void (*reset) (HTMLObject *o);
	
	gint (*calc_min_width) (HTMLObject *o);
	
	gint (*calc_preferred_width) (HTMLObject *o);

	void (*calc_absolute_pos) (HTMLObject *o, gint x, gint y);

	const gchar * (*get_url) (HTMLObject *o);

	const gchar * (*get_target) (HTMLObject *o);

	HTMLAnchor * (*find_anchor) (HTMLObject *o, const gchar *name,
				     gint *x, gint *y);

	void (* set_bg_color) (HTMLObject *o, GdkColor *color);
};


extern HTMLObjectClass html_object_class;


void html_object_type_init (void);
void html_object_init (HTMLObject *o, HTMLObjectClass *klass);
void html_object_class_init (HTMLObjectClass *klass, HTMLType type);
HTMLObject *html_object_new (void);
void html_object_destroy (HTMLObject *o);

void html_object_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y,
		       gint width, gint height, gint tx, gint ty);
HTMLFitType html_object_fit_line (HTMLObject *o, gboolean start_of_line, 
				  gboolean first_run, gint width_left);
void html_object_calc_size (HTMLObject *o, HTMLObject *parent);
void html_object_set_max_ascent (HTMLObject *o, gint a);
void html_object_set_max_descent (HTMLObject *o, gint d);
void html_object_destroy (HTMLObject *o);
void html_object_set_max_width (HTMLObject *o, gint max_width);
void html_object_reset (HTMLObject *o);
gint html_object_calc_min_width (HTMLObject *o);
gint html_object_calc_preferred_width (HTMLObject *o);
void html_object_calc_absolute_pos (HTMLObject *o, gint x, gint y);
const gchar *html_object_get_url (HTMLObject *o);
const gchar *html_object_get_target (HTMLObject *o);
HTMLAnchor *html_object_find_anchor (HTMLObject *o, const gchar *name,
				     gint *x, gint *y);
void html_object_set_bg_color (HTMLObject *o, GdkColor *color);

#endif /* _HTMLOBJECT_H_ */

