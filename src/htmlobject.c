/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
    Copyright (C) 1997 Torben Weis (weis@kde.org)
    Copyright (C) 1999 Helix Code, Inc.

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
#include "htmlcursor.h"
#include "debug.h"


HTMLObjectClass html_object_class;

#define HO_CLASS(x) HTML_OBJECT_CLASS (HTML_OBJECT (x)->klass)


/* HTMLObject virtual methods.  */

static void
destroy (HTMLObject *o)
{
	if (o->font != NULL)
		html_font_destroy (o->font);

	g_free (o);
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      HTMLCursor *cursor,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	if (cursor->object == o)
		html_painter_draw_cursor (p, o->x + tx, o->y + ty, o->ascent, o->descent);
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
}

static void
set_max_ascent (HTMLObject *o, gint a)
{
}
	
static void
set_max_descent (HTMLObject *o, gint d)
{
}
	
static void
set_max_width (HTMLObject *o, gint max_width)
{
	o->max_width = max_width;
}

static void
reset (HTMLObject *o)
{
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

/* FIXME TODO this does not take account for the `child' argument yet, so we
   actually relayout all the objects.  */
static gboolean
relayout (HTMLObject *self,
	  HTMLEngine *engine,
	  HTMLObject *child)
{
	/* FIXME int types of this stuff might change in `htmlobject.h',
           remember to sync.  */
	gshort prev_width;
	gint prev_ascent, prev_descent;

	prev_width = self->width;
	prev_ascent = self->ascent;
	prev_descent = self->descent;

	html_object_reset (self);

	/* FIXME extreme ugliness here.  It should use the `parent' member in
           the object, instead of asking us to provide it ourselves.  */
	html_object_calc_size (self, self->parent);

	if (prev_width == self->width
	    && prev_ascent == self->ascent
	    && prev_descent == self->descent) {
		g_print ("relayout: %s %p did not change.\n",
			 html_type_name (HTML_OBJECT_TYPE (self)),
			 self);
		return FALSE;
	} else {
		g_print ("relayout: %s %p changed.\n",
			 html_type_name (HTML_OBJECT_TYPE (self)),
			 self);	
		if (self->parent == NULL) {
			/* FIXME resize the widget, e.g. scrollbars and such.  */
			html_engine_queue_draw (engine, self);

			/* FIXME extreme ugliness.  */
			self->x = 0;
			self->y = self->ascent;
		} else {
			/* Relayout our parent starting from us.  */
			if (! html_object_relayout (self->parent, engine, self))
				html_engine_queue_draw (engine, self);
		}

		return TRUE;
	}
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
	klass->get_url = get_url;
	klass->get_target = get_target;
	klass->find_anchor = find_anchor;
	klass->set_bg_color = set_bg_color;
	klass->mouse_event = mouse_event;
	klass->check_point = check_point;
	klass->relayout = relayout;
}

void
html_object_init (HTMLObject *o,
		  HTMLObjectClass *klass)
{
	o->klass = klass;

	o->parent = NULL;
	o->prev = NULL;
	o->next = NULL;

	o->x = 0;
	o->y = 0;

	o->ascent = 0;
	o->descent = 0;
	o->font = NULL;

	o->width = 0;
	o->max_width = 0;
	o->percent = 0;

	o->flags = HTML_OBJECT_FLAG_FIXEDWIDTH; /* FIXME Why? */

	o->redraw_pending = FALSE;
}

HTMLObject *
html_object_new (HTMLObject *parent)
{
	HTMLObject *o;
	
	o = g_new0 (HTMLObject, 1);
	html_object_init (o, &html_object_class);

	return o;
}

void
html_object_set_parent (HTMLObject *o,
			HTMLObject *parent)
{
	/* If this happens, something must be broken: we are not moving
           elements around at the moment.  */
	g_return_if_fail (o->parent == NULL);

	o->parent = parent;
}

void
html_object_calc_abs_position (HTMLObject *o,
			       gint *x_return, gint *y_return)
{
	HTMLObject *p;

	g_return_if_fail (o != NULL);

	*x_return = 0;
	*y_return = 0;

	for (p = o; p != NULL; p = p->parent) {
		*x_return += p->x;
		*y_return += p->y;
	}
}


/* Virtual methods.  */

void
html_object_destroy (HTMLObject *o)
{
	(* HO_CLASS (o)->destroy) (o);
}

void
html_object_draw (HTMLObject *o, HTMLPainter *p, HTMLCursor *cursor,
		  gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	(* HO_CLASS (o)->draw) (o, p, cursor, x, y, width, height, tx, ty);
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
	return (* HO_CLASS (o)->calc_min_width) (o);
}

gint
html_object_calc_preferred_width (HTMLObject *o)
{
	return (* HO_CLASS (o)->calc_preferred_width) (o);
}

const gchar *
html_object_get_url (HTMLObject *o)
{
	return (* HO_CLASS (o)->get_url) (o);
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
	return (* HO_CLASS (self)->check_point) (self, x, y);
}

gboolean
html_object_relayout (HTMLObject *self,
		      HTMLEngine *engine,
		      HTMLObject *child)
{
	return (* HO_CLASS (self)->relayout) (self, engine, child);
}
