/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 2000 Helix Code, Inc.
   
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

#include "htmlrule.h"


HTMLRuleClass html_rule_class;


/* HTMLObject methods.  */

static void
set_max_width (HTMLObject *o, HTMLPainter *painter, gint max_width)
{
	o->max_width = max_width;

	/* FIXME why the `if'? */
	if (! (o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH))
		o->width = max_width;
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	html_object_calc_size (o, painter);

	if (o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH)
		return o->width;
	
	return 1;
}

static void
draw (HTMLObject *o,
      HTMLPainter *p, 
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLRule *rule;
	guint w;
	gint xp, yp;

	rule = HTML_RULE (o);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;
	
	xp = o->x + tx;
	yp = o->y + ty;

	if (o->percent == 0) {
		w = o->width;
	} else {
		w = (o->width * o->percent) / 100;
		switch (rule->halign) {
		case HTML_HALIGN_LEFT:
			break;
		case HTML_HALIGN_CENTER:
		case HTML_HALIGN_NONE:
			/* Default is `align=center' according to the specs.  */
			xp += (o->width - w) / 2;
			break;
		case HTML_HALIGN_RIGHT:
			xp += o->width - w;
			break;
		default:
			g_warning ("Unknown HTMLRule alignment %d.", rule->halign);
		}
	}

	if (rule->shade)
		html_painter_draw_shade_line (p, xp, yp, w);
	else
		html_painter_fill_rect (p, xp, yp, w, o->ascent - 6);
}

static gboolean
accepts_cursor (HTMLObject *self)
{
	return TRUE;
}

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	return html_engine_save_output_string (state, "\n<HR>\n");
}


void
html_rule_type_init (void)
{
	html_rule_class_init (&html_rule_class, HTML_TYPE_RULE, sizeof (HTMLRule));
}

void
html_rule_class_init (HTMLRuleClass *klass,
		      HTMLType type,
		      guint object_size)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type, object_size);

	object_class->draw = draw;
	object_class->set_max_width = set_max_width;
	object_class->calc_min_width = calc_min_width;
	object_class->accepts_cursor = accepts_cursor;
	object_class->save = save;
}

void
html_rule_init (HTMLRule *rule,
		HTMLRuleClass *klass,
		gint max_width,
		gint percent,
		gint size,
		gboolean shade,
		HTMLHAlignType halign)
{
	HTMLObject *object;

	object = HTML_OBJECT (rule);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	if (size < 1)
		size = 1;	/* FIXME why? */

	object->ascent = 6 + size / 2;
	object->descent = 6 + size / 2 + size % 2;

	object->width = max_width;
	object->percent = percent;

	rule->shade = shade;
	rule->halign = halign;

	if (percent > 0) {
		object->width = max_width * percent / 100;
		object->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;
	}
}

HTMLObject *
html_rule_new (gint max_width,
	       gint percent,
	       gint size,
	       gboolean shade,
	       HTMLHAlignType halign)
{
	HTMLRule *rule;

	rule = g_new (HTMLRule, 1);
	html_rule_init (rule, &html_rule_class, max_width, percent,
			size, shade, halign);

	return HTML_OBJECT (rule);
}
