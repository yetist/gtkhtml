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

#include <string.h>

#include "htmlobject.h"
#include "htmlfont.h"
#include "htmlclue.h"
#include "htmltext.h"
#include "htmltextmaster.h"
#include "htmlclueflow.h"
#include "htmlcluev.h"
#include "htmlpainter.h"
#include "htmlrule.h"
#include "htmlclue.h"
#include "debug.h"


HTMLObjectClass html_object_class;

#define HO_CLASS(x) HTML_OBJECT_CLASS (HTML_OBJECT (x)->klass)


/* HTMLObject virtual methods.  */

static void
destroy (HTMLObject *o)
{
	/* FIXME do we kill the `next' member as well?  This depends on whether
           we want objects to work like lists or not.  */

	if (o->font != NULL)
		html_font_destroy (o->font);

	g_free (o);
}

static void
draw (HTMLObject *o, HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
#if 0
	g_warning ("`%s' does not implement `draw'.",
		   html_type_name (HTML_OBJECT_TYPE (o)));
#endif
}

static HTMLFitType
fit_line (HTMLObject *o,
	  gboolean start_of_line,
	  gboolean first_run,
	  gint width_left)
{
	return HTML_FIT_COMPLETE;
}

static void
calc_size (HTMLObject *o,
	   HTMLObject *parent)
{
#if 0
	g_warning ("`%s' does not implement `calc_size'.",
		   html_type_name (HTML_OBJECT_TYPE (o)));
#endif
}

static void
set_max_ascent (HTMLObject *o, gint a)
{
#if 0
	g_warning ("`%s' does not implement `set_max_ascent'.",
		   html_type_name (HTML_OBJECT_TYPE (o)));
#endif
}
	
static void
set_max_descent (HTMLObject *o, gint d)
{
#if 0
	g_warning ("`%s' does not implement `set_max_descent'.",
		   html_type_name (HTML_OBJECT_TYPE (o)));
#endif
}
	
static void
set_max_width (HTMLObject *o, gint max_width)
{
	o->max_width = max_width;
}

static void
reset (HTMLObject *o)
{
	/* FIXME: Do something here */
}

static gint
calc_min_width (HTMLObject *o)
{
	return o->width;
}

static gint
calc_preferred_width (HTMLObject *o)
{
	return o->width;
}

static void
calc_absolute_pos (HTMLObject *o, gint x, gint y)
{
	o->abs_x = x + o->x;
	o->abs_y = y + o->y - o->ascent;
}

static const gchar *
get_url (HTMLObject *o)
{
	return NULL;
}

static const gchar *
get_target (HTMLObject *o)
{
	return NULL;
}

static HTMLAnchor *
find_anchor (HTMLObject *o,
	     const gchar *name,
	     gint *x, gint *y)
{
	return NULL;
}

static void
set_bg_color (HTMLObject *o,
	      GdkColor *color)
{
}

static HTMLObject *
mouse_event (HTMLObject *o,
	     gint x, gint y, gint button, gint state)
{
	return NULL;
}

static HTMLObject*
check_point (HTMLObject *o, gint _x, gint _y )
{
	if ( _x >= o->x && _x < o->x + o->width )
		if ( _y > o->y - o->ascent && _y < o->y + o->descent + 1 )
			return o;
    
	return 0L;
}


/* Class initialization.  */

void
html_object_type_init (void)
{
	html_object_class_init (&html_object_class, HTML_TYPE_OBJECT);
}

void
html_object_class_init (HTMLObjectClass *klass,
			HTMLType type)
{
	g_return_if_fail (klass != NULL);

	/* Set type.  */
	klass->type = type;

	/* Install virtual methods.  */
	klass->destroy = destroy;
	klass->draw = draw;
	klass->fit_line = fit_line;
	klass->calc_size = calc_size;
	klass->set_max_ascent = set_max_ascent;
	klass->set_max_descent = set_max_descent;
	klass->set_max_width = set_max_width;
	klass->reset = reset;
	klass->calc_min_width = calc_min_width;
	klass->calc_preferred_width = calc_preferred_width;
	klass->calc_absolute_pos = calc_absolute_pos;
	klass->get_url = get_url;
	klass->get_target = get_target;
	klass->find_anchor = find_anchor;
	klass->set_bg_color = set_bg_color;
	klass->mouse_event = mouse_event;
	klass->check_point = check_point;
}

void
html_object_init (HTMLObject *o,
		  HTMLObjectClass *klass)
{
	o->klass = klass;

	o->x = 0;
	o->y = 0;

	o->ascent = 0;
	o->descent = 0;
	o->font = NULL;

	o->width = 0;
	o->max_width = 0;
	o->percent = 0;

	o->flags = HTML_OBJECT_FLAG_FIXEDWIDTH; /* FIXME Why? */

	o->abs_x = 0;
	o->abs_y = 0;

	o->next = NULL;
}

HTMLObject *
html_object_new (void)
{
	HTMLObject *o;
	
	o = g_new0 (HTMLObject, 1);
	html_object_init (o, &html_object_class);

	return o;
}


void
html_object_destroy (HTMLObject *o)
{
	(* HO_CLASS (o)->destroy) (o);
}

void
html_object_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y,
		  gint width, gint height, gint tx, gint ty)
{
#if 0
	static guint level = 0;
	guint i;

	level++;
	for (i = 0; i < level; i++)
		putchar (' ');

	printf ("Drawing %s %d %d %d %d %d %d\n",
		html_type_name (o->klass->type),
		x, y, width, height, tx, ty);
#endif

	(* HO_CLASS (o)->draw) (o, p, x, y, width, height, tx, ty);

#if 0
	level--;
#endif
}

HTMLFitType
html_object_fit_line (HTMLObject *o, gboolean start_of_line, 
		      gboolean first_run, gint width_left)
{
	return (* HO_CLASS (o)->fit_line) (o, start_of_line,
					   first_run, width_left);
}

void
html_object_calc_size (HTMLObject *o, HTMLObject *parent)
{
	(* HO_CLASS (o)->calc_size) (o, parent);
}

void
html_object_set_max_ascent (HTMLObject *o, gint a)
{
	(* HO_CLASS (o)->set_max_ascent) (o, a);
}

void
html_object_set_max_descent (HTMLObject *o, gint d)
{
	(* HO_CLASS (o)->set_max_descent) (o, d);
}

void
html_object_set_max_width (HTMLObject *o, gint max_width)
{
	(* HO_CLASS (o)->set_max_width) (o, max_width);
}

void
html_object_reset (HTMLObject *o)
{
	(* HO_CLASS (o)->reset) (o);
}

gint
html_object_calc_min_width (HTMLObject *o)
{
	gint value = (* HO_CLASS (o)->calc_min_width) (o);

	return value;
}

gint
html_object_calc_preferred_width (HTMLObject *o)
{
	return (* HO_CLASS (o)->calc_preferred_width) (o);
}

void
html_object_calc_absolute_pos (HTMLObject *o, gint x, gint y)
{
	return (* HO_CLASS (o)->calc_absolute_pos) (o, x, y);
}

const gchar *
html_object_get_url (HTMLObject *o)
{
	const gchar *url;

	url = (* HO_CLASS (o)->get_url) (o);

	return url;
}

const gchar *
html_object_get_target (HTMLObject *o)
{
	return (* HO_CLASS (o)->get_target) (o);
}

HTMLAnchor *
html_object_find_anchor (HTMLObject *o,
			 const gchar *name,
			 gint *x, gint *y)
{
	return (* HO_CLASS (o)->find_anchor) (o, name, x, y);
}

void
html_object_set_bg_color (HTMLObject *o, GdkColor *color)
{
	(* HO_CLASS (o)->set_bg_color) (o, color);
}

HTMLObject *
html_object_mouse_event (HTMLObject *self, gint x, gint y,
			 gint button, gint state)
{
	return (* HO_CLASS (self)->mouse_event) (self, x, y, button, state);
}

HTMLObject *
html_object_check_point (HTMLObject *self, gint x, gint y)
{
	HTMLObject *object;
	gint i;

#if 0
	static gint counter = 2;
	for (i = 0; i < counter; i++)
		putchar (' ');
	printf ("check_point %s %d %d %d %d\n", html_type_name (HTML_OBJECT_TYPE (self)),
		(int) self->x, (int) self->y, (int) self->width, (int) self->ascent);
	counter++;
#endif

	object = (* HO_CLASS (self)->check_point) (self, x, y);

#if 0
	counter--;
#endif

	return object;
}
