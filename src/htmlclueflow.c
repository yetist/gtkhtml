/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

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

#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluealigned.h"
#include "htmltext.h"
#include "htmlhspace.h"
#include "htmlvspace.h"


/* Indentation width.  */
#define INDENT_UNIT 25

#define BULLET_SIZE 4
#define BULLET_HPAD  4
#define BULLET_VPAD  2

HTMLClueFlowClass html_clueflow_class;


static gboolean
style_is_item (HTMLClueFlowStyle style)
{
	switch (style) {
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return TRUE;
	default:
		return FALSE;
	}
}


static void
set_max_width (HTMLObject *o, gint max_width)
{
	HTMLObject *obj;
	guint indent;

	o->max_width = max_width;
	indent = HTML_CLUEFLOW (o)->level * INDENT_UNIT;
	
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next)
		html_object_set_max_width (obj, o->max_width - indent);
}

static gint
calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint w;
	gint tempMinWidth;

	tempMinWidth = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		w = html_object_calc_min_width (obj);
		if (w > tempMinWidth)
			tempMinWidth = w;
	}

	tempMinWidth += HTML_CLUEFLOW (o)->level * INDENT_UNIT;

	return tempMinWidth;
}

/* EP CHECK: should be mostly OK.  */
/* FIXME: But it's awful.  Too big and ugly.  */
static void
calc_size (HTMLObject *o,
	   HTMLObject *parent)
{
	HTMLVSpace *vspace;
	HTMLClue *clue;
	HTMLObject *obj;
	HTMLObject *line;
	HTMLClearType clear;
	gboolean newLine;
	gboolean firstLine;
	gint lmargin, rmargin;
	gint indent;
	gint oldy;
	gint w, a, d;

	clue = HTML_CLUE (o);

	obj = clue->head;
	line = clue->head;
	clear = HTML_CLEAR_NONE;
	indent = HTML_CLUEFLOW (o)->level * INDENT_UNIT;

	o->ascent = 0;
	o->descent = 0;
	o->width = 0;

	lmargin = html_clue_get_left_margin (HTML_CLUE (parent), o->y);
	if (indent > lmargin)
		lmargin = indent;
	rmargin = html_clue_get_right_margin (HTML_CLUE (parent), o->y);

	w = lmargin;
	a = 0;
	d = 0;
	newLine = FALSE;
	firstLine = TRUE;

	while (obj != 0) {
		if (obj->flags & HTML_OBJECT_FLAG_NEWLINE) {
			if (!a)
				a = obj->ascent;
			if (!a && (obj->descent > d))
				d = obj->descent;
			newLine = TRUE;
			vspace = HTML_VSPACE (obj);
			clear = vspace->clear;
			obj = obj->next;
		} else if (obj->flags & HTML_OBJECT_FLAG_SEPARATOR) {
			obj->x = w;

			/* FIXME I am not sure why this check is here, but it
                           causes a stand-alone hspace in a clueflow to be
                           given zero height.  -- EP */
			if (TRUE /* w != lmargin */) {
				w += obj->width;
				if (obj->ascent > a)
					a = obj->ascent;
				if (obj->descent > d)
					d = obj->descent;
			}
			obj = obj->next;
		} else if (obj->flags & HTML_OBJECT_FLAG_ALIGNED) {
			HTMLClueAligned *c = (HTMLClueAligned *)obj;
			
			if (! html_clue_appended (HTML_CLUE (parent), HTML_CLUE (c))) {
				html_object_calc_size (obj, NULL);

				if (HTML_CLUE (c)->halign == HTML_HALIGN_LEFT) {
					obj->x = lmargin;
					obj->y = o->ascent + obj->ascent;

					html_clue_append_left_aligned
						(HTML_CLUE (parent),
						 HTML_CLUE (c));
				} else {
					obj->x = rmargin - obj->width;
					obj->y = o->ascent + obj->ascent;
					
					html_clue_append_right_aligned
						(HTML_CLUE (parent),
						 HTML_CLUE (c));
				}
			}
			obj = obj->next;
		}
		/* This is a normal object.  We must add all objects upto the next
		   separator/newline/aligned object. */
		else {
			gint runWidth;
			HTMLObject *run;

			/* By setting "newLine = true" we move the complete run to
			   a new line.
			   We shouldn't set newLine if we are at the start of a line.  */

			runWidth = 0;
			run = obj;
			while ( run
				&& ! (run->flags & HTML_OBJECT_FLAG_SEPARATOR)
				&& ! (run->flags & HTML_OBJECT_FLAG_NEWLINE)
				&& ! (run->flags & HTML_OBJECT_FLAG_ALIGNED)) {
				HTMLFitType fit;
			  
				run->max_width = rmargin - lmargin;
				fit = html_object_fit_line (run,
							    w + runWidth == lmargin,
							    obj == line,
							    rmargin - runWidth - w);

				if (fit == HTML_FIT_NONE) {
					newLine = TRUE;
					break;
				}
				
				html_object_calc_size (run, o);
				runWidth += run->width;

				/* If this run cannot fit in the allowed area, break it
				   on a non-separator */
				if ((run != obj) && (runWidth > rmargin - lmargin))
					break;
				
				if (run->ascent > a)
					a = run->ascent;
				if (run->descent > d)
					d = run->descent;

				run = run->next;

				if (fit == HTML_FIT_PARTIAL) {
					/* Implicit separator */
					break;
				}

				if (runWidth > rmargin - lmargin)
					break;
			}

			/* If these objects do not fit in the current line and we are
			   not at the start of a line then end the current line in
			   preparation to add this run in the next pass. */
			if (w > lmargin && w + runWidth > rmargin)
				newLine = TRUE;

			if (! newLine) {
				gint new_y, new_lmargin, new_rmargin;
				guint indent;

				indent = HTML_CLUEFLOW (o)->level * INDENT_UNIT;

				/* Check if the run fits in the current flow area 
				   especially with respect to its height.
				   If not, find a rectangle with height a+b. The size of
				   the rectangle will be rmargin-lmargin. */

				html_clue_find_free_area (HTML_CLUE (parent),
							  o->y,
							  line->width,
							  a+d,
							  indent,
							  &new_y, &new_lmargin,
							  &new_rmargin);
				
				if (new_y != o->y
				    || new_lmargin > lmargin
				    || new_rmargin < rmargin) {
					
					/* We did not get the location we expected 
					   we start building our current line again */
					/* We got shifted downwards by "new_y - y"
					   add this to both "y" and "ascent" */

					new_y -= o->y;
					o->ascent += new_y;

					o->y += new_y;

					lmargin = new_lmargin;
					if (indent > lmargin)
						lmargin = indent;
					rmargin = new_rmargin;
					obj = line;
					
					/* Reset this line */
					w = lmargin;
					d = 0;
					a = 0;

					newLine = FALSE;
					clear = HTML_CLEAR_NONE;
				} else {
					while (obj != run) {
						obj->x = w;
						w += obj->width;
						obj = obj->next;
					}
				}
			}
			
		}
		
		/* if we need a new line, or all objects have been processed
		   and need to be aligned. */
		if ( newLine || !obj) {
			int extra;

			extra = 0;

			o->ascent += a + d;

			/* o->y += a + d; FIXME */

			if (w > o->width)
				o->width = w;
			
			if (clue->halign == HTML_HALIGN_CENTER) {
				extra = (rmargin - w) / 2;
				if (extra < 0)
					extra = 0;
			}
			else if (clue->halign == HTML_HALIGN_RIGHT) {
				extra = rmargin - w;
				if (extra < 0)
					extra = 0;
			}
			
			while (line != obj) {
				if (!(line->flags & HTML_OBJECT_FLAG_ALIGNED)) {
					line->y = o->ascent - d;
					html_object_set_max_ascent (line, a);
					html_object_set_max_descent (line, d);
					if (clue->halign == HTML_HALIGN_CENTER
					    || clue->halign == HTML_HALIGN_RIGHT) {
						line->x += extra;
					}
				}
				line = line->next;
			}

			oldy = o->y;
			
			if (clear == HTML_CLEAR_ALL) {
				int new_lmargin, new_rmargin;
				
				html_clue_find_free_area (HTML_CLUE (parent), oldy,
							  o->max_width,
							  1, 0,
							  &o->y,
							  &new_lmargin,
							  &new_rmargin);
			} else if (clear == HTML_CLEAR_LEFT) {
				o->y = html_clue_get_left_clear (HTML_CLUE (parent), oldy);
			} else if (clear == HTML_CLEAR_RIGHT) {
				o->y = html_clue_get_right_clear (HTML_CLUE (parent), oldy);
			}

			o->ascent += o->y - oldy;

			lmargin = html_clue_get_left_margin (HTML_CLUE (parent),
							     o->y);
			if (HTML_CLUEFLOW (o)->level * INDENT_UNIT > lmargin)
				lmargin = HTML_CLUEFLOW (o)->level * INDENT_UNIT;
			rmargin = html_clue_get_right_margin (HTML_CLUE (parent),
							      o->y);

			w = lmargin;
			d = 0;
			a = 0;
			
			newLine = FALSE;
			clear = HTML_CLEAR_NONE;
		}
	}
	
	if (o->width < o->max_width)
		o->width = o->max_width;
}

static gint
calc_preferred_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint maxw = 0, w = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		if (!(obj->flags & HTML_OBJECT_FLAG_NEWLINE)) {
			w += html_object_calc_preferred_width (obj);
		} else {
			if (w > maxw)
				maxw = w;
			w = 0;
		}
	}
	
	if (w > maxw)
		maxw = w;

	return maxw + HTML_CLUEFLOW (o)->level * INDENT_UNIT;
}

static void
draw (HTMLObject *self,
      HTMLPainter *painter,
      HTMLCursor *cursor,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLClueFlow *clueflow;
	HTMLObject *first;

	if (y > self->y + self->descent
	    || y + height < self->y - self->ascent)
		return;

	clueflow = HTML_CLUEFLOW (self);
	first = HTML_CLUE (self)->head;

	if (first != NULL
	    && clueflow->level > 0
	    && style_is_item (clueflow->style)) {
		gint xp, yp;

		html_painter_set_pen (painter, clueflow->font->textColor);

		xp = self->x + first->x - BULLET_SIZE - BULLET_HPAD;
		yp = self->y - self->ascent + first->y - BULLET_SIZE - BULLET_VPAD;

		xp += tx, yp += ty;

		if (clueflow->level & 1)
			html_painter_fill_rect (painter, xp, yp, BULLET_SIZE, BULLET_SIZE);
		else
			html_painter_draw_rect (painter, xp, yp, BULLET_SIZE, BULLET_SIZE);
	}

	(* HTML_OBJECT_CLASS (&html_clue_class)->draw) (self, painter, cursor,
							x, y, width, height, tx, ty);
}


void
html_clueflow_type_init (void)
{
	html_clueflow_class_init (&html_clueflow_class, HTML_TYPE_CLUEFLOW);
}

void
html_clueflow_class_init (HTMLClueFlowClass *klass,
			  HTMLType type)
{
	HTMLClueClass *clue_class;
	HTMLObjectClass *object_class;

	clue_class = HTML_CLUE_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_clue_class_init (clue_class, type);

	object_class->calc_size = calc_size;
	object_class->set_max_width = set_max_width;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->draw = draw;
}

void
html_clueflow_init (HTMLClueFlow *clueflow,
		    HTMLClueFlowClass *klass,
		    HTMLFont *font,
		    HTMLClueFlowStyle style,
		    guint8 level)
{
	HTMLObject *object;
	HTMLClue *clue;

	object = HTML_OBJECT (clueflow);
	clue = HTML_CLUE (clueflow);

	html_clue_init (clue, HTML_CLUE_CLASS (klass));

	object->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;

	clue->valign = HTML_VALIGN_BOTTOM;
	clue->halign = HTML_HALIGN_LEFT;

	clueflow->font = font;
	clueflow->style = style;
	clueflow->level = level;
}

HTMLObject *
html_clueflow_new (HTMLFont *font,
		   HTMLClueFlowStyle style,
		   guint8 level)
{
	HTMLClueFlow *clueflow;

	clueflow = g_new (HTMLClueFlow, 1);
	html_clueflow_init (clueflow, &html_clueflow_class,
			    font, style, level);

	return HTML_OBJECT (clueflow);
}


/* Clue splitting (for editing).  */

/**
 * html_clue_split:
 * @clue: 
 * @child: 
 * 
 * Remove @child and its successors from @clue, and create a new clue
 * containing them.  The new clue has the same properties as the original clue.
 * 
 * Return value: A pointer to the new clue.
 **/
HTMLClueFlow *
html_clueflow_split (HTMLClueFlow *clue,
		     HTMLObject *child)
{
	HTMLClueFlow *new;
	HTMLObject *prev;

	g_return_val_if_fail (clue != NULL, NULL);
	g_return_val_if_fail (child != NULL, NULL);

	/* Create the new clue.  */

	new = HTML_CLUEFLOW (html_clueflow_new (clue->font, clue->style, clue->level));

	/* Remove the children from the original clue.  */

	prev = child->prev;
	if (prev != NULL)
		prev->next = NULL;
	else
		HTML_CLUE (clue)->head = NULL;
	child->prev = NULL;

	/* The last text element in an HTMLClueFlow must be followed by an
           hidden hspace.  */

	if (prev != NULL && html_object_is_text (prev)) {
		HTMLObject *hspace;

		hspace = html_hspace_new (HTML_TEXT (prev)->font, TRUE);
		html_clue_append (HTML_CLUE (clue), hspace);
	}

	/* Put the children into the new clue.  */

	html_clue_append (HTML_CLUE (new), child);

	/* Return the new clue.  */

	return new;
}
