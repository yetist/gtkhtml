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
#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluealigned.h"
#include "htmlvspace.h"


HTMLClueFlowClass html_clueflow_class;


static void
set_max_width (HTMLObject *o, gint max_width)
{
	HTMLObject *obj;

	o->max_width = max_width;
	
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next)
		html_object_set_max_width
			(obj, o->max_width - HTML_CLUEFLOW (o)->indent);

}

static gint
calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint w;
	gint tempMinWidth=0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		w = html_object_calc_min_width (obj);
		if (w > tempMinWidth)
			tempMinWidth = w;
	}
	tempMinWidth += HTML_CLUEFLOW (o)->indent;

	return tempMinWidth;
}

/* EP CHECK: should be mostly OK, except for the `FIXME' line.  */
static void
calc_size (HTMLObject *o,
	   HTMLObject *parent)
{

	HTMLVSpace *vspace;
	HTMLClue *clue;
	HTMLObject *obj;
	HTMLObject *line;
	gboolean newLine;
	gint lmargin, rmargin;
	gint oldy;
	gint w, a, d;
	HTMLClearType clear;

	clue = HTML_CLUE (o);

	obj = clue->head;
	line = clue->head;
	clear = HTML_CLEAR_NONE;

	o->ascent = 0;
	o->descent = 0;
	o->width = 0;
	lmargin = html_clue_get_left_margin (HTML_CLUE (parent), o->y);
	if (HTML_CLUEFLOW (o)->indent > lmargin)
		lmargin = HTML_CLUEFLOW (o)->indent;
	rmargin = html_clue_get_right_margin (HTML_CLUE (parent), o->y);

	w = lmargin;
	a = 0;
	d = 0;
	newLine = FALSE;

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
			
		}
		else if (obj->flags & HTML_OBJECT_FLAG_SEPARATOR) {
			obj->x = w;

			if (w != lmargin) {
				w += obj->width;
				if (obj->ascent > a)
					a = obj->ascent;
				if (obj->descent > d)
					d = obj->descent;
			}
			obj = obj->next;
		}
		else if (obj->flags & HTML_OBJECT_FLAG_ALIGNED) {
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
				fit = html_object_fit_line (run, (w + runWidth == lmargin),
											(obj == line),
											rmargin - runWidth - w);

				if (fit == HTML_FIT_NONE) {
					newLine = TRUE;
					break;
				}
				
				html_object_calc_size (run, o);
				runWidth += run->width;

				/* If this run cannot fit in the allowed area, break it
				   on a non-separator */
				if ((run != obj) && (runWidth > rmargin - lmargin)) {
					break;
				}
				
				if (run->ascent > a)
					a = run->ascent;
				if (run->descent > d)
					d = run->descent;

				run = run->next;

				if (fit == HTML_FIT_PARTIAL) {
					/* Implicit separator */
					break;
				}

				if (runWidth > rmargin - lmargin) {
					break;
				}
				
			}

			/* If these objects do not fit in the current line and we are
			   not at the start of a line then end the current line in
			   preparation to add this run in the next pass. */
			if (w > lmargin && w + runWidth > rmargin) {
				newLine = TRUE;
			}

			if (!newLine) {
				gint new_y, new_lmargin, new_rmargin;

				/* Check if the run fits in the current flow area 
				   especially with respect to its height.
				   If not, find a rectangle with height a+b. The size of
				   the rectangle will be rmargin-lmargin. */

				html_clue_find_free_area (HTML_CLUE (parent),
										  o->y,
										  line->width,
										  a+d,
										  HTML_CLUEFLOW (o)->indent,
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
					o->y += new_y;
					o->ascent += new_y;

					lmargin = new_lmargin;
					if (HTML_CLUEFLOW (o)->indent > lmargin)
						lmargin = HTML_CLUEFLOW (o)->indent;
					rmargin = new_rmargin;
					obj = line;
					
					/* Reset this line */
					w = lmargin;
					d = 0;
					a = 0;

					newLine = FALSE;
					clear = HTML_CLEAR_NONE;
				}
				else {
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
			int extra = 0;
			o->ascent += a + d;
			o->y += a + d;
			
			if (w > o->width) {
				o->width = w;
			}
			
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
			if (HTML_CLUEFLOW (o)->indent > lmargin)
				lmargin = HTML_CLUEFLOW (o)->indent;
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
		}
		else {
			if (w > maxw)
				maxw = w;
			w = 0;
		}
	}
	
	if (w > maxw)
		maxw = w;

	return maxw + HTML_CLUEFLOW (o)->indent;
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
}

void
html_clueflow_init (HTMLClueFlow *clueflow,
		    HTMLClueFlowClass *klass,
		    gint x, gint y,
		    gint max_width, gint percent)
{
	HTMLObject *object;
	HTMLClue *clue;

	object = HTML_OBJECT (clueflow);
	clue = HTML_CLUE (clueflow);

	html_clue_init (clue, HTML_CLUE_CLASS (klass));

	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->percent = percent;
	object->width = max_width;
	object->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;

	clue->valign = HTML_VALIGN_BOTTOM;
	clue->halign = HTML_HALIGN_LEFT;
	clue->head = 0;
	clue->tail = 0;
	clue->curr = 0;

	clueflow->indent = 0;
}

HTMLObject *
html_clueflow_new (gint x, gint y, gint max_width, gint percent)
{
	HTMLClueFlow *clueflow;

	clueflow = g_new (HTMLClueFlow, 1);
	html_clueflow_init (clueflow, &html_clueflow_class,
			    x, y, max_width, percent);

	return HTML_OBJECT (clueflow);
}
