/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 1999, 2000 Helix Code, Inc.

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

/* This is the object that defines a paragraph in the HTML document.  */

/* WARNING: it must always be the child of a clue.  */

#include <config.h>
#include <ctype.h>
#include <string.h>
#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluealigned.h"
#include "htmltext.h"
#include "htmlvspace.h"
#include "htmllinktextmaster.h"
#include "htmltextslave.h"	/* FIXME */
#include "htmlsearch.h"
#include "htmlentity.h"
#include "htmlengine-edit.h"


HTMLClueFlowClass html_clueflow_class;
static HTMLClueClass *parent_class = NULL;

#define HCF_CLASS(x) HTML_CLUEFLOW_CLASS (HTML_OBJECT (x)->klass)


static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	HTML_CLUEFLOW (dest)->style = HTML_CLUEFLOW (self)->style;
	HTML_CLUEFLOW (dest)->level = HTML_CLUEFLOW (self)->level;
}

static guint
calc_padding (HTMLPainter *painter)
{
	guint ascent, descent;

	/* FIXME maybe this should depend on the style.  */
	ascent = html_painter_calc_ascent (painter, GTK_HTML_FONT_STYLE_SIZE_3, NULL);
	descent = html_painter_calc_descent (painter, GTK_HTML_FONT_STYLE_SIZE_3, NULL);

	return ascent + descent;
}

static guint
calc_indent_unit (HTMLPainter *painter)
{
	guint ascent, descent;

	ascent = html_painter_calc_ascent (painter, GTK_HTML_FONT_STYLE_SIZE_3, NULL);
	descent = html_painter_calc_descent (painter, GTK_HTML_FONT_STYLE_SIZE_3, NULL);

	return (ascent + descent) * 3;
}

static guint
calc_bullet_size (HTMLPainter *painter)
{
	guint ascent, descent;

	ascent = html_painter_calc_ascent (painter, GTK_HTML_FONT_STYLE_SIZE_3, NULL);
	descent = html_painter_calc_descent (painter, GTK_HTML_FONT_STYLE_SIZE_3, NULL);

	return (ascent + descent) / 3;
}


static gboolean
is_item (HTMLClueFlow *flow)
{
	switch (flow->style) {
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return TRUE;
	default:
		return FALSE;
	}
}

static gboolean
is_header (HTMLClueFlow *flow)
{
	switch (flow->style) {
	case HTML_CLUEFLOW_STYLE_H1:
	case HTML_CLUEFLOW_STYLE_H2:
	case HTML_CLUEFLOW_STYLE_H3:
	case HTML_CLUEFLOW_STYLE_H4:
	case HTML_CLUEFLOW_STYLE_H5:
	case HTML_CLUEFLOW_STYLE_H6:
		return TRUE;
	default:
		return FALSE;
	}
}

static guint
get_pre_padding (HTMLClueFlow *flow,
		 guint pad)
{
	HTMLObject *prev_object;

	prev_object = HTML_OBJECT (flow)->prev;
	if (prev_object == NULL)
		return 0;

	if (HTML_OBJECT_TYPE (prev_object) == HTML_TYPE_CLUEFLOW) {
		HTMLClueFlow *prev;

		prev = HTML_CLUEFLOW (prev_object);
		if (prev->level > 0 && flow->level == 0)
			return pad;

		if (flow->style == HTML_CLUEFLOW_STYLE_PRE
		    && prev->style != HTML_CLUEFLOW_STYLE_PRE
		    && ! is_header (prev))
			return pad;

		if (is_header (flow) && ! is_header (prev))
			return pad;

		return 0;
	}

	if (! is_header (flow) && flow->level == 0)
		return 0;

	return pad;
}

static guint
get_post_padding (HTMLClueFlow *flow,
		  guint pad)
{
	HTMLObject *next_object;

	next_object = HTML_OBJECT (flow)->next;
	if (next_object == NULL)
		return 0;

	if (HTML_OBJECT_TYPE (next_object) == HTML_TYPE_CLUEFLOW) {
		HTMLClueFlow *next;

		next = HTML_CLUEFLOW (next_object);
		if (next->level > 0 && flow->level == 0)
			return pad;

		if (flow->style == HTML_CLUEFLOW_STYLE_PRE
		    && next->style != HTML_CLUEFLOW_STYLE_PRE
		    && ! is_header (next))
			return pad;

		if (is_header (flow) &&  ! is_header (next))
			return pad;

		return 0;
	}

	if (! is_header (flow) && flow->level == 0)
		return 0;

	return pad;
}

static void
add_pre_padding (HTMLClueFlow *flow,
		 guint pad)
{
	guint real_pad;

	real_pad = get_pre_padding (flow, pad);

	HTML_OBJECT (flow)->ascent += real_pad;
	HTML_OBJECT (flow)->y += real_pad;
}

static void
add_post_padding (HTMLClueFlow *flow,
		  guint pad)
{
	guint real_pad;

	real_pad = get_post_padding (flow, pad);

	HTML_OBJECT (flow)->ascent += real_pad;
	HTML_OBJECT (flow)->y += real_pad;
}

static guint
get_indent (HTMLClueFlow *flow,
	    HTMLPainter *painter)
{
	guint level;
	guint indent;

	level = flow->level;

	if (level > 0 || ! is_item (flow))
		indent = level * calc_indent_unit (painter);
	else
		indent = 2 * calc_bullet_size (painter);

	return indent;
}


/* HTMLObject methods.  */
static void
set_max_width (HTMLObject *o,
	       HTMLPainter *painter,
	       gint max_width)
{
	HTMLObject *obj;
	guint indent;

	o->max_width = max_width;

	indent = get_indent (HTML_CLUEFLOW (o), painter);
	
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		html_object_set_max_width (obj, painter, o->max_width - indent);
	}
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	HTMLObject *cur;
	gint min_width = 0;
	gint w = 0;
	gboolean add;

	add = (HTML_CLUEFLOW (o)->style == HTML_CLUEFLOW_STYLE_PRE
	       || HTML_CLUEFLOW (o)->style == HTML_CLUEFLOW_STYLE_NOWRAP);

	cur = HTML_CLUE (o)->head;
	while (cur) {
		w += (add) ? html_object_calc_preferred_width (cur, painter) : html_object_calc_min_width (cur, painter);
		if (!add || cur->flags & HTML_OBJECT_FLAG_NEWLINE || !cur->next) {
			if (min_width < w) min_width = w;
			w = 0;
		}
		cur = cur->next;
	}

	/* printf ("flow min width: %d\n", min_width + get_indent (HTML_CLUEFLOW (o), painter)); */

	return min_width + get_indent (HTML_CLUEFLOW (o), painter);
}

static gint
set_line_x (HTMLObject **obj, HTMLObject *run, gint x, gboolean *changed)
{
	while (*obj != run) {
		if ((*obj)->x != x) {
			(*obj)->x = x;   
			*changed = TRUE;
		}
		x   += (*obj)->width;
		*obj = (*obj)->next;
	}
	return x;
}

/* EP CHECK: should be mostly OK.  */
/* FIXME: But it's awful.  Too big and ugly.  */
static gboolean
calc_size (HTMLObject *o,
	   HTMLPainter *painter)
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
	guint padding;
	gboolean changed;
	gint old_ascent, old_descent, old_width;
	gint runWidth = 0;
	gboolean have_valign_top;

	html_clueflow_remove_text_slaves (HTML_CLUEFLOW (o));

	changed = FALSE;
	old_ascent = o->ascent;
	old_descent = o->descent;
	old_width = o->width;

	clue = HTML_CLUE (o);

	obj = clue->head;
	line = clue->head;
	clear = HTML_CLEAR_NONE;
	indent = get_indent (HTML_CLUEFLOW (o), painter);

	o->ascent = 0;
	o->descent = 0;
	o->width = 0;

	padding = calc_padding (painter);
	add_pre_padding (HTML_CLUEFLOW (o), padding);

	lmargin = html_object_get_left_margin (o->parent, o->y);
	if (indent > lmargin)
		lmargin = indent;
	rmargin = html_object_get_right_margin (o->parent, o->y);

	w = lmargin;
	a = 0;
	d = 0;
	newLine = FALSE;
	firstLine = TRUE;

	have_valign_top = FALSE;

	while (obj != NULL) {
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
			if (obj->x != w) {
				obj->x = w;
				changed = TRUE;
			}
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
			
			if (! html_clue_appended (HTML_CLUE (o->parent), HTML_CLUE (c))) {
				html_object_calc_size (obj, painter);

				if (HTML_CLUE (c)->halign == HTML_HALIGN_LEFT) {
					if (obj->x != lmargin) {
						obj->x = lmargin;
						changed = TRUE;
					}
					if (obj->y != o->ascent + obj->ascent + a + d) {
						obj->y = o->ascent + obj->ascent + a + d;
						changed = TRUE;
					}
					html_clue_append_left_aligned (HTML_CLUE (o->parent),
								       HTML_CLUE (c));

					lmargin = html_object_get_left_margin (o->parent, o->y);

					if (indent > lmargin)
						lmargin = indent;

					if (a + d == 0)
						w = lmargin;
					else
						w = runWidth + lmargin;
				} else {
					if (obj->x != rmargin - obj->width) {
						obj->x = rmargin - obj->width;
						changed = TRUE;
					}
					if (obj->y != o->ascent + obj->ascent + a + d) {
						obj->y = o->ascent + obj->ascent + a + d;
						changed = TRUE;
					}
					
					html_clue_append_right_aligned (HTML_CLUE (o->parent),
									HTML_CLUE (c));

					rmargin = html_object_get_right_margin (o->parent, o->y);
				}
			}

			obj = obj->next;
		}
		/* This is a normal object.  We must add all objects upto the next
		   separator/newline/aligned object. */
		else {
			HTMLObject *run;

			/* By setting "newLine = true" we move the complete run
			   to a new line.  We shouldn't set newLine if we are
			   at the start of a line.  */
			runWidth = 0;
			run = obj;
			
			while ( run
				&& ! (run->flags & HTML_OBJECT_FLAG_SEPARATOR)
				&& ! (run->flags & HTML_OBJECT_FLAG_NEWLINE)
				&& ! (run->flags & HTML_OBJECT_FLAG_ALIGNED)) {
				HTMLFitType fit;
				HTMLVAlignType valign;
				gint width_left;

				width_left = rmargin - runWidth - w;
				fit = html_object_fit_line (run,
							    painter,
							    w + runWidth == lmargin,
							    run == line || (HTML_OBJECT_TYPE (run) == HTML_TYPE_TEXTSLAVE
									    && HTML_OBJECT (HTML_TEXT_SLAVE (run)->owner)
									    == line
									    && !HTML_TEXT_SLAVE (run)->posStart),
							    width_left);

				if (fit == HTML_FIT_NONE) {
					w = set_line_x (&obj, run, w, &changed);
					newLine = TRUE;
					break;
				}

				html_object_calc_size (run, painter);
				runWidth += run->width;

				valign = html_object_get_valign (run);

				/* Algorithm for dealing vertical alignment.
				   Elements with `HTML_VALIGN_BOTTOM' and
				   `HTML_VALIGN_CENTER' can be handled
				   immediately.	 Objects with
				   `HTML_VALIGN_TOP', instead, need to know the
				   total height of the line, so need to be
				   handled last.  */

				switch (valign) {
				case HTML_VALIGN_CENTER: {
					gint height;
					gint half_height;

					height = run->ascent + run->descent;
					half_height = height / 2;
					if (half_height > a)
						a = half_height;
					if (height - half_height > d)
						d = height - half_height;
					break;
				}
				case HTML_VALIGN_BOTTOM:
					if (run->ascent > a)
						a = run->ascent;
					if (run->descent > d)
						d = run->descent;
					break;
				case HTML_VALIGN_TOP:
					have_valign_top = TRUE;
					/* Do nothing for now.	*/
					break;
				default:
					g_assert_not_reached ();
				}

				run = run->next;

				if (fit == HTML_FIT_PARTIAL) {
					/* Implicit separator */
					break;
				}
			}

			if (! newLine) {
				gint new_y, new_lmargin, new_rmargin;

				/* Check if the run fits in the current flow area 
				   especially with respect to its height.
				   If not, find a rectangle with height a+b. The size of
				   the rectangle will be rmargin-lmargin. */

				html_clue_find_free_area (HTML_CLUE (o->parent),
							  o->y,
							  rmargin - lmargin,
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
					w = set_line_x (&obj, run, w, &changed);
					/* we've used up this line so insert a newline */
					newLine = TRUE;
				}
				lmargin = html_object_get_left_margin (o->parent, o->y);
				
				if (indent > lmargin)
					lmargin = indent;

				rmargin = html_object_get_right_margin (o->parent, o->y);
			}
		}
		
		/* if we need a new line, or all objects have been processed
		   and need to be aligned. */
		if ( newLine || !obj) {
			int extra;

			extra = 0;

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

			o->ascent += a + d;
			o->y += a + d;

			/* Update the height for `HTML_VALIGN_TOP' objects.  */
			if (have_valign_top) {
				HTMLObject *p;

				for (p = line; p != obj; p = p->next) {
					gint height, rest;

					if (html_object_get_valign (p) != HTML_VALIGN_TOP)
						continue;

					height = p->ascent + p->descent;
					rest = height - a;

					if (rest > d) {
						o->ascent += rest - d;
						o->y += rest - d;
						d = rest;
					}
				}

				have_valign_top = FALSE;
			}

			for (; line != obj; line = line->next) {
				if (line->flags & HTML_OBJECT_FLAG_ALIGNED)
					continue;

				/* FIXME max_ascent/max_descent -- not quite
				   sure what they are for. */

				switch (html_object_get_valign (line)) {
				case HTML_VALIGN_BOTTOM:
					line->y = o->ascent - d;
					html_object_set_max_ascent (line, painter, a);
					html_object_set_max_descent (line, painter, d);
					break;
				case HTML_VALIGN_CENTER:
					line->y = o->ascent - a - d + line->ascent;
					break;
				case HTML_VALIGN_TOP:
					line->y = o->ascent - a - d + line->ascent;
					break;
				default:
					g_assert_not_reached ();
				}

				if (clue->halign == HTML_HALIGN_CENTER
				    || clue->halign == HTML_HALIGN_RIGHT)
					line->x += extra;
			}

			oldy = o->y;
			
			if (clear == HTML_CLEAR_ALL) {
				int new_lmargin, new_rmargin;
				
				html_clue_find_free_area (HTML_CLUE (o->parent),
							  oldy,
							  o->max_width,
							  1, 0,
							  &o->y,
							  &new_lmargin,
							  &new_rmargin);
			} else if (clear == HTML_CLEAR_LEFT) {
				o->y = html_clue_get_left_clear (HTML_CLUE (o->parent), oldy);
			} else if (clear == HTML_CLEAR_RIGHT) {
				o->y = html_clue_get_right_clear (HTML_CLUE (o->parent), oldy);
			}

			o->ascent += o->y - oldy;

			lmargin = html_object_get_left_margin (o->parent, o->y);
			if (indent > lmargin)
				lmargin = indent;
			rmargin = html_object_get_right_margin (o->parent, o->y);

			w = lmargin;
			d = 0;
			a = 0;
			
			newLine = FALSE;
			clear = HTML_CLEAR_NONE;
		
		}
	}
	
       
	if (o->width < o->max_width)
		o->width = o->max_width;

#if 0
	if (o->width > rmargin - o->x)
		o->width = rmargin - o->x;
#endif
	add_post_padding (HTML_CLUEFLOW (o), padding);

	if (o->ascent != old_ascent || o->descent != old_descent || o->width != old_width)
		changed = TRUE;

	return changed;
}

static gint
calc_preferred_width (HTMLObject *o,
		      HTMLPainter *painter)
{
	HTMLObject *obj;
	gint maxw = 0, w = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		if (!(obj->flags & HTML_OBJECT_FLAG_NEWLINE)) {
			w += html_object_calc_preferred_width (obj, painter);
		}

		if (obj->flags & HTML_OBJECT_FLAG_NEWLINE || !html_object_next_not_slave (obj)) {
			HTMLObject *eol = (obj->flags & HTML_OBJECT_FLAG_NEWLINE) ? html_object_prev_not_slave (obj) : obj;

			/* remove trailing space width on the end of line */
			if (HTML_OBJECT_TYPE (eol) == HTML_TYPE_TEXTMASTER
			    || HTML_OBJECT_TYPE (eol) == HTML_TYPE_LINKTEXTMASTER) {
				w -= html_text_master_trail_space_width (HTML_TEXT_MASTER (eol), painter);
			}

			if (w > maxw)
				maxw = w;
			w = 0;
		}
	}

	return maxw + get_indent (HTML_CLUEFLOW (o), painter);
}

static void
draw (HTMLObject *self,
      HTMLPainter *painter,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLClueFlow *clueflow;
	HTMLObject *first;
	guint bullet_size;

	if (y > self->y + self->descent || y + height < self->y - self->ascent)
		return;

	clueflow = HTML_CLUEFLOW (self);
	first = HTML_CLUE (self)->head;

	if (first != NULL && is_item (clueflow)) {
		gint xp, yp;

		bullet_size = calc_bullet_size (painter);

		/* FIXME pen color?  */
		
		xp = self->x + first->x - 2 * bullet_size;
		yp = self->y - self->ascent + first->y - (first->ascent + bullet_size) / 2 - bullet_size;

		xp += tx, yp += ty;

		html_painter_set_pen (painter,
				      &html_colorset_get_color_allocated (painter, HTMLTextColor)->color);

		if (clueflow->level == 0 || (clueflow->level & 1) != 0)
			html_painter_fill_rect (painter, xp, yp, bullet_size, bullet_size);
		else
			html_painter_draw_rect (painter, xp, yp, bullet_size, bullet_size);
	}

	(* HTML_OBJECT_CLASS (&html_clue_class)->draw) (self,
							painter, 
							x, y,
							width, height,
							tx, ty);
}

static void
draw_background (HTMLObject *self,
		 HTMLPainter *p,
		 gint x, gint y,
		 gint width, gint height,
		 gint tx, gint ty)
{
	
	html_object_draw (self->parent, p,
			  x + self->parent->x,
			  y + self->parent->y - self->parent->ascent,
			  width, height,
			  tx - self->parent->x,
			  ty - self->parent->y + self->parent->ascent);
	
	html_object_draw_background (self->parent, p,
				     x + self->parent->x,
				     y + self->parent->y - self->parent->ascent,
				     width, height,
				     tx - self->parent->x,
				     ty - self->parent->y + self->parent->ascent);
}


static HTMLObject*
check_point (HTMLObject *self,
	     HTMLPainter *painter,
	     gint x, gint y,
	     guint *offset_return,
	     gboolean for_cursor)
{
	HTMLObject *obj;
	HTMLObject *p;
	HTMLObject *pnext;
	HTMLClue *clue;

	if (x < self->x || x >= self->x + self->width
	    || y < self->y - self->ascent || y >= self->y + self->descent)
		return NULL;

	clue = HTML_CLUE (self);

	x = x - self->x;
	y = y - self->y + self->ascent;

	for (p = clue->head; p != NULL; p = pnext) {
		gint x1, y1;

		pnext = p->next;

		obj = html_object_check_point (p, painter, x, y, offset_return, for_cursor);
		if (obj != NULL)
			return obj;

		if (! for_cursor)
			continue;

		if (p->prev == NULL && (x < self->x || y < self->y - self->ascent + 1)) {
			x1 = self->x;
			y1 = self->y - self->ascent + 1;
			obj = html_object_check_point (p, painter, x1, y1, offset_return, for_cursor);
			if (obj != NULL)
				return obj;
		} else if (pnext == NULL || (pnext->y != p->y
					     && y >= p->y - p->ascent + 1
					     && y <= p->y + p->descent)) {
			HTMLObject *obj1;

			x1 = p->x + p->width - 1;
			y1 = p->y;

			if (HTML_OBJECT_TYPE (p) != HTML_TYPE_TEXTSLAVE) {
				obj1 = p;
			} else {
				for (obj1 = p;
				     HTML_OBJECT_TYPE (obj1) == HTML_TYPE_TEXTSLAVE;
				     obj1 = obj1->prev)
					;
			}

			obj = html_object_check_point (obj1, painter, x1, y1, offset_return, for_cursor);
			if (obj != NULL)
				return obj;
		}
	}

	return NULL;
}

static void
append_selection_string (HTMLObject *self,
			 GString *buffer)
{
	if (! self->selected)
		return;

	g_string_append_c (buffer, '\n');
}


/* Saving support.  */

static gboolean
write_indent (HTMLEngineSaveState *state)
{
	return html_engine_save_output_string (state, "    ");
}

static const char *
get_tag (HTMLClueFlow *flow)
{
	switch (flow->style) {
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
		return "ul";
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return "ol";
	case HTML_CLUEFLOW_STYLE_NORMAL:
	case HTML_CLUEFLOW_STYLE_H1:
	case HTML_CLUEFLOW_STYLE_H2:
	case HTML_CLUEFLOW_STYLE_H3:
	case HTML_CLUEFLOW_STYLE_H4:
	case HTML_CLUEFLOW_STYLE_H5:
	case HTML_CLUEFLOW_STYLE_H6:
	case HTML_CLUEFLOW_STYLE_ADDRESS:
	case HTML_CLUEFLOW_STYLE_PRE:
	case HTML_CLUEFLOW_STYLE_NOWRAP:
	default:
		return "blockquote";
	}
}

static gboolean
write_indentation_tags (HTMLEngineSaveState *state,
			guint last_value,
			guint new_value,
			const gchar *tag)
{
	guint i, j;

	if (new_value == last_value)
		return TRUE;

	if (! html_engine_save_output_string (state, "\n"))
		return FALSE;

	if (new_value > last_value) {
		for (i = last_value; i < new_value; i++) {
			for (j = 0; j < i; j++) {
				if (! write_indent (state))
					return FALSE;
			}

			if (! html_engine_save_output_string (state, "<%s>\n", tag))
				return FALSE;
		}
	} else {
		for (i = last_value; i > new_value; i--) {
			if (i > 1) {
				for (j = 0; j < i - 1; j++) {
					if (! html_engine_save_output_string (state, "    "))
						return FALSE;
				}
			}

			if (! html_engine_save_output_string (state, "</%s>\n", tag))
				return FALSE;
		}
	}

	return TRUE;
}

static gboolean
write_pre_tags (HTMLClueFlow *self,
		HTMLEngineSaveState *state)
{
	HTMLClueFlow *prev;
	const char *prev_tag, *curr_tag;

	prev = HTML_CLUEFLOW (HTML_OBJECT (self)->prev);
	if (prev != NULL && prev->level == self->level && prev->style == self->style) {
		if (! is_item (self))
			return html_engine_save_output_string (state, "<br>\n");
		else
			return TRUE;
	}

	if (prev != NULL)
		prev_tag = get_tag (prev);
	else
		prev_tag = NULL;

	curr_tag = get_tag (self);

	if (prev != NULL && strcmp (prev_tag, curr_tag) == 0) {
		write_indentation_tags (state, prev->level, self->level, prev_tag);
	} else {
		if (prev != NULL)
			write_indentation_tags (state, prev->level, 0, prev_tag);
		write_indentation_tags (state, 0, self->level, curr_tag);
	}

	return TRUE;
}

static gboolean
write_post_tags (HTMLClueFlow *self,
		 HTMLEngineSaveState *state)
{
	const char *tag;

	if (HTML_OBJECT (self)->next != NULL)
		return TRUE;

	tag = get_tag (self);
	write_indentation_tags (state, self->level, 0, tag);

	return TRUE;
}

static const gchar *
get_tag_for_style (const HTMLClueFlow *flow)
{
	switch (flow->style) {
	case HTML_CLUEFLOW_STYLE_NORMAL:
		return NULL;
	case HTML_CLUEFLOW_STYLE_H1:
		return "h1";
	case HTML_CLUEFLOW_STYLE_H2:
		return "h2";
	case HTML_CLUEFLOW_STYLE_H3:
		return "h3";
	case HTML_CLUEFLOW_STYLE_H4:
		return "h4";
	case HTML_CLUEFLOW_STYLE_H5:
		return "h5";
	case HTML_CLUEFLOW_STYLE_H6:
		return "h6";
	case HTML_CLUEFLOW_STYLE_ADDRESS:
		return "address";
	case HTML_CLUEFLOW_STYLE_PRE:	
		return "pre";
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return "li";
	default:
		g_warning ("Unknown HTMLClueFlowStyle %d", flow->style);
		return NULL;
	}
}

static const gchar *
halign_to_string (HTMLHAlignType halign)
{
	switch (halign) {
	case HTML_HALIGN_RIGHT:
		return "right";
	case HTML_HALIGN_CENTER:
		return "center";
	case HTML_HALIGN_LEFT:
	case HTML_HALIGN_NONE:
	default:
		return "left";
	}
}

static gboolean 
is_similar (HTMLObject *self, HTMLObject *friend)
{
	if (friend &&  HTML_OBJECT_TYPE (friend) == HTML_TYPE_CLUEFLOW) {
		if ((HTML_CLUEFLOW (friend)->style == HTML_CLUEFLOW (self)->style)
		    && (HTML_CLUEFLOW (friend)->level == HTML_CLUEFLOW (self)->level)) {
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	HTMLClueFlow *clueflow;
	HTMLHAlignType halign;
	const gchar *tag;
	gboolean start = TRUE, end = TRUE;
	gint i;

	clueflow = HTML_CLUEFLOW (self);
	halign = HTML_CLUE (self)->halign;

	if (! write_pre_tags (clueflow, state))
		return FALSE;

	tag = get_tag_for_style (clueflow);

	if (is_similar (self, self->prev))
		start = FALSE;

	if (is_similar (self, self->next))
		end = FALSE;

	/* Indentation.  */
	if (clueflow->style != HTML_CLUEFLOW_STYLE_PRE) {
		for (i = 0; i < clueflow->level; i++) {
			if (! write_indent (state))
				return FALSE;
		}
	}
	
	/* Alignment tag.  */
	if (halign != HTML_HALIGN_NONE && halign != HTML_HALIGN_LEFT) {
		if (! html_engine_save_output_string (state, "<div alignx=%s>\n", halign_to_string (halign)))
			return FALSE;
	}

	/* Start tag.  */
	if (tag != NULL && (start || is_item (clueflow))
	    && (! html_engine_save_output_string (state, "<%s>", tag)))
		return FALSE;

	/* Paragraph's content.  */
	if (! HTML_OBJECT_CLASS (&html_clue_class)->save (self, state))
		return FALSE;

	/* End tag.  */
	if (tag && (end || is_item (clueflow))) {
		if (! html_engine_save_output_string (state, "</%s>", tag))
			return FALSE;
	}

	/* Close alignment tag.  */
	if (halign != HTML_HALIGN_NONE && halign != HTML_HALIGN_LEFT) {
		if (! html_engine_save_output_string (state, "</div>\n"))
			return FALSE;
	} else if (tag != NULL
		   && HTML_CLUEFLOW (self)->style != HTML_CLUEFLOW_STYLE_PRE) {
		if (! html_engine_save_output_string (state, "\n"))
			return FALSE;
	}

	return write_post_tags (HTML_CLUEFLOW (self), state);
}

static gboolean
save_plain (HTMLObject *self,
	    HTMLEngineSaveState *state)
{
	HTMLClueFlow *clueflow;

	clueflow = HTML_CLUEFLOW (self);

	if (get_pre_padding (clueflow, 1) > 0)
		if (! html_engine_save_output_string (state, "\n"))
			return FALSE;

	if (clueflow->level > 0) {
		gint i;

		for (i = 0; i < (gint) clueflow->level; i++) {
			if (! html_engine_save_output_string (state, "\t"))
				return FALSE;
		}
	}

	/* Paragraph's content.  */
	if (! HTML_OBJECT_CLASS (&html_clue_class)->save_plain (self, state))
		return FALSE;

	if (!html_engine_save_output_string (state, "\n"))
		return FALSE;
	
	if (get_post_padding (clueflow, 1) > 0)
		if (! html_engine_save_output_string (state, "\n"))
			return FALSE;

	return TRUE;
}


static gint
check_page_split (HTMLObject *self,
		  gint y)
{
	HTMLClue *clue;
	HTMLObject *p;
	gint last_base;

	clue = HTML_CLUE (self);

	last_base = 0;
	for (p = clue->head; p != NULL; p = p->next) {
		gint base;

		base = p->y + p->descent;

		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTMASTER
		    || HTML_OBJECT_TYPE (p) == HTML_TYPE_LINKTEXTMASTER)
			continue;

		if (base > y)
			return last_base;

		last_base = base;
	}

	return y;
}


static GtkHTMLFontStyle
get_default_font_style (const HTMLClueFlow *self)
{
	switch (self->style) {
	case HTML_CLUEFLOW_STYLE_NORMAL:
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
	case HTML_CLUEFLOW_STYLE_NOWRAP:
		return GTK_HTML_FONT_STYLE_SIZE_3;
	case HTML_CLUEFLOW_STYLE_ADDRESS:
		return GTK_HTML_FONT_STYLE_SIZE_3 | GTK_HTML_FONT_STYLE_ITALIC;
	case HTML_CLUEFLOW_STYLE_PRE:
		return GTK_HTML_FONT_STYLE_SIZE_3 | GTK_HTML_FONT_STYLE_FIXED;
	case HTML_CLUEFLOW_STYLE_H1:
		return GTK_HTML_FONT_STYLE_SIZE_6 | GTK_HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H2:
		return GTK_HTML_FONT_STYLE_SIZE_5 | GTK_HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H3:
		return GTK_HTML_FONT_STYLE_SIZE_4 | GTK_HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H4:
		return GTK_HTML_FONT_STYLE_SIZE_3 | GTK_HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H5:
		return GTK_HTML_FONT_STYLE_SIZE_2 | GTK_HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H6:
		return GTK_HTML_FONT_STYLE_SIZE_1 | GTK_HTML_FONT_STYLE_BOLD;
	default:
		g_warning ("Unexpected HTMLClueFlow style %d", self->style);
		return GTK_HTML_FONT_STYLE_DEFAULT;
	}
}

static void
search_set_info (HTMLObject *cur, HTMLSearch *info, guint pos, guint len)
{
	guint text_len = 0;
	guint cur_len;

	info->found_len = len;

	if (info->found) {
		g_list_free (info->found);
		info->found = NULL;
	}

	while (cur) {
		if (HTML_OBJECT_TYPE (cur) == HTML_TYPE_TEXTMASTER
		    || HTML_OBJECT_TYPE (cur) == HTML_TYPE_LINKTEXTMASTER) {
			cur_len = HTML_TEXT (cur)->text_len;
			if (text_len + cur_len > pos) {
				if (!info->found) {
					info->start_pos = pos - text_len;
				}
				info->found = g_list_append (info->found, cur);
			}
			text_len += cur_len;
			if (text_len >= pos+info->found_len) {
				info->stop_pos = cur_len - (text_len - pos - info->found_len);
				info->last     = HTML_OBJECT (cur);
				return;
			}
		} else if (HTML_OBJECT_TYPE (cur) != HTML_TYPE_TEXTSLAVE) {
			break;
		}		
		cur = cur->next;
	}

	g_assert_not_reached ();
}

/* search text objects ([TextMaster, LinkTextMaster], TextSlave*) */
static gboolean
search_text (HTMLObject **beg, HTMLSearch *info)
{
	HTMLObject *cur = *beg;
	HTMLObject *end = cur;
	HTMLObject *head;
	guchar *par, *pp;
	guint text_len;
	guint eq_len;
	gint  pos;
	gboolean retval = FALSE;

	/* printf ("search flow look for \"text\" %s\n", info->text); */

	/* first get flow text_len */
	text_len = 0;
	while (cur) {
		if (HTML_OBJECT_TYPE (cur) == HTML_TYPE_TEXTMASTER
		    || HTML_OBJECT_TYPE (cur) == HTML_TYPE_LINKTEXTMASTER) {
			text_len += HTML_TEXT (cur)->text_len;
			end = cur;
		} else if (HTML_OBJECT_TYPE (cur) != HTML_TYPE_TEXTSLAVE) {
			break;
		}
		cur = (info->forward) ? cur->next : cur->prev;
	}

	if (text_len > 0) {
		par = g_new (gchar, text_len+1);
		par [text_len] = 0;

		pp = (info->forward) ? par : par+text_len;

		/* now fill par with text */
		head = cur = (info->forward) ? *beg : end;
		cur = *beg;
		while (cur) {
			if (HTML_OBJECT_TYPE (cur) == HTML_TYPE_TEXTMASTER
			    || HTML_OBJECT_TYPE (cur) == HTML_TYPE_LINKTEXTMASTER) {
				if (!info->forward) {
					pp -= HTML_TEXT (cur)->text_len;
				}
				strncpy (pp, HTML_TEXT (cur)->text, HTML_TEXT (cur)->text_len);
				if (info->forward) {
					pp += HTML_TEXT (cur)->text_len;
				}
			} else if (HTML_OBJECT_TYPE (cur) != HTML_TYPE_TEXTSLAVE) {
				break;
			}		
			cur = (info->forward) ? cur->next : cur->prev;
		}

		/* set eq_len and pos counters */
		eq_len = 0;
		if (info->found) {
			pos = info->start_pos + ((info->forward) ? 1 : -1);
		} else {
			pos = (info->forward) ? 0 : text_len - 1;
		}

		/* FIXME make shorter text instead */
		if (!info->forward)
			par [pos+1] = 0;

		if ((info->forward && pos < text_len)
		    || (!info->forward && pos>0)) {
			if (info->reb) {
				/* regex search */
				gint rv;
#ifndef HAVE_GNU_REGEX
				regmatch_t match;
				guchar *p=par+pos;

				/* replace &nbsp;'s with spaces */
				while (*p) {
					if (*p == ENTITY_NBSP) {
						*p = ' ';
					}
					p += (info->forward) ? 1 : -1;
				}

				while ((info->forward && pos < text_len)
				       || (!info->forward && pos >= 0)) {
					rv = regexec (info->reb,
						      par + pos,
						      1, &match, 0);
					if (rv == 0) {
						search_set_info (head, info, pos + match.rm_so, match.rm_eo - match.rm_so);
						retval = TRUE;
						break;
					}
					pos += (info->forward) ? 1 : -1;
				}
#else
				rv = re_search (info->reb, par, text_len, pos,
						(info->forward) ? text_len-pos : -pos, NULL);
				if (rv>=0) {
					guint found_pos = rv;
					rv = re_match (info->reb, par, text_len, found_pos, NULL);
					if (rv < 0) {
						g_warning ("re_match (...) error");
					}
					search_set_info (head, info, found_pos, rv);
					retval = TRUE;
				} else {
					if (rv < -1) {
						g_warning ("re_search (...) error");
					}
				}
#endif
			} else {
				/* substring search - simple one - could be improved
				   go thru par and look for info->text */
				while (par [pos]) {
					if (info->trans [(guchar) info->text
							[(info->forward) ? eq_len : info->text_len - eq_len - 1]]
					    == info->trans [par [pos]]) {
						eq_len++;
						if (eq_len == info->text_len) {
							search_set_info (head, info, pos - ((info->forward)
											    ? eq_len-1 : 0), info->text_len);
							retval=TRUE;
							break;
						}
					} else {
						pos += (info->forward) ? -eq_len : eq_len;
						eq_len = 0;
					}
					pos += (info->forward) ? 1 : -1;
				}
			}
		}
		g_free (par);
	}

	*beg = cur;

	return retval;
}

static gboolean
search (HTMLObject *obj, HTMLSearch *info)
{
	HTMLClue *clue = HTML_CLUE (obj);
	HTMLObject *cur;
	gboolean next = FALSE;

	/* does last search end here? */
	if (info->found) {
		cur  = HTML_OBJECT (info->found->data);
		next = TRUE;
	} else {
		/* search_next? */
		if (html_search_child_on_stack (info, obj)) {
			cur  = html_search_pop (info);
			cur  = (info->forward) ? cur->next : cur->prev;
			next = TRUE;
		} else {
			/* normal search */
			cur  = (info->forward) ? clue->head : clue->tail;
		}
	}
	while (cur) {
		if (HTML_OBJECT_TYPE (cur) == HTML_TYPE_TEXTMASTER
		    || HTML_OBJECT_TYPE (cur) == HTML_TYPE_LINKTEXTMASTER) {
			if (search_text (&cur, info)) {
				return TRUE;
			}
		} else {
			html_search_push (info, cur);
			if (html_object_search (cur, info))
				return TRUE;
			html_search_pop (info);
			cur = (info->forward) ? cur->next : cur->prev;
		}
		if (info->found) {
			g_list_free (info->found);
			info->found = NULL;
			info->start_pos = 0;
		}
	}

	if (next) {
		return html_search_next_parent (info);
	}

	return FALSE;
}

static gboolean
search_next (HTMLObject *obj, HTMLSearch *info)
{
	return FALSE;
}


void
html_clueflow_type_init (void)
{
	html_clueflow_class_init (&html_clueflow_class, HTML_TYPE_CLUEFLOW, sizeof (HTMLClueFlow));
}

void
html_clueflow_class_init (HTMLClueFlowClass *klass,
			  HTMLType type,
			  guint size)
{
	HTMLClueClass *clue_class;
	HTMLObjectClass *object_class;

	clue_class = HTML_CLUE_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_clue_class_init (clue_class, type, size);

	/* FIXME destroy */

	object_class->copy = copy;
	object_class->calc_size = calc_size;
	object_class->set_max_width = set_max_width;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->draw = draw;
	object_class->draw_background = draw_background;
	object_class->save = save;
	object_class->save_plain = save_plain;
	object_class->check_page_split = check_page_split;
	object_class->check_point = check_point;
	object_class->append_selection_string = append_selection_string;
	object_class->search = search;
	object_class->search_next = search_next;

	klass->get_default_font_style = get_default_font_style;

	parent_class = &html_clue_class;
}

void
html_clueflow_init (HTMLClueFlow *clueflow,
		    HTMLClueFlowClass *klass,
		    HTMLClueFlowStyle style,
		    guint8 level)
{
	HTMLObject *object;
	HTMLClue *clue;

	object = HTML_OBJECT (clueflow);
	clue = HTML_CLUE (clueflow);

	html_clue_init (clue, HTML_CLUE_CLASS (klass));

	object->flags &= ~HTML_OBJECT_FLAG_FIXEDWIDTH;

	clue->valign = HTML_VALIGN_BOTTOM;
	clue->halign = HTML_HALIGN_LEFT;

	clueflow->style = style;
	clueflow->level = level; 
}

HTMLObject *
html_clueflow_new (HTMLClueFlowStyle style,
		   guint8 level)
{
	HTMLClueFlow *clueflow;

	clueflow = g_new (HTMLClueFlow, 1);
	html_clueflow_init (clueflow, &html_clueflow_class, style, level);

	return HTML_OBJECT (clueflow);
}


/* Virtual methods.  */

GtkHTMLFontStyle
html_clueflow_get_default_font_style (const HTMLClueFlow *self)
{
	g_return_val_if_fail (self != NULL, GTK_HTML_FONT_STYLE_DEFAULT);

	return (* HCF_CLASS (self)->get_default_font_style) (self);
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

	new = HTML_CLUEFLOW (html_clueflow_new (clue->style, clue->level));

	/* Remove the children from the original clue.  */

	prev = child->prev;
	if (prev != NULL) {
		prev->next = NULL;
		HTML_CLUE (clue)->tail = prev;
	} else {
		HTML_CLUE (clue)->head = NULL;
		HTML_CLUE (clue)->tail = NULL;
	}

	child->prev = NULL;
	html_object_change_set (HTML_OBJECT (clue), HTML_CHANGE_ALL);

	/* Put the children into the new clue.  */

	html_clue_append (HTML_CLUE (new), child);

	/* Return the new clue.  */

	return new;
}


static void
relayout_and_draw (HTMLObject *object,
		   HTMLEngine *engine)
{
	if (engine == NULL)
		return;

	html_object_relayout (object, engine, NULL);
	html_engine_queue_draw (engine, object);
}

/* This performs a relayout of the object when the indentation level
   has changed.  In this case, we need to relayout the previous
   paragraph and the following one, because their padding might change
   after the level change. */
static void
relayout_with_siblings (HTMLClueFlow *flow,
			HTMLEngine *engine)
{
	if (engine == NULL)
		return;

	/* FIXME this is ugly and inefficient.  */

	if (HTML_OBJECT (flow)->prev != NULL)
		relayout_and_draw (HTML_OBJECT (flow)->prev, engine);

	relayout_and_draw (HTML_OBJECT (flow), engine);

	if (HTML_OBJECT (flow)->next != NULL)
		relayout_and_draw (HTML_OBJECT (flow)->next, engine);
}


void
html_clueflow_set_style (HTMLClueFlow *flow,
			 HTMLEngine *engine,
			 HTMLClueFlowStyle style)
{
	g_return_if_fail (flow != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	flow->style = style;

	relayout_with_siblings (flow, engine);
}

HTMLClueFlowStyle
html_clueflow_get_style (HTMLClueFlow *flow)
{
	g_return_val_if_fail (flow != NULL, HTML_CLUEFLOW_STYLE_NORMAL);

	return flow->style;
}

void
html_clueflow_set_halignment (HTMLClueFlow *flow,
			      HTMLEngine *engine,
			      HTMLHAlignType alignment)
{
	g_return_if_fail (flow != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	HTML_CLUE (flow)->halign = alignment;

	relayout_and_draw (HTML_OBJECT (flow), engine);
}

HTMLHAlignType
html_clueflow_get_halignment (HTMLClueFlow *flow)
{
	g_return_val_if_fail (flow != NULL, HTML_HALIGN_NONE);

	return HTML_CLUE (flow)->halign;
}

void
html_clueflow_indent (HTMLClueFlow *flow,
		      HTMLEngine *engine,
		      gint indentation)
{
	g_return_if_fail (flow != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	if (indentation == 0)
		return;

	if (indentation > 0) {
		flow->level += indentation;
	} else if ((- indentation) < flow->level) {
		flow->level += indentation;
	} else if (flow->level != 0) {
		flow->level = 0;
	} else {
		/* No change.  */
		return;
	}

	relayout_with_siblings (flow, engine);
}

void
html_clueflow_set_indentation (HTMLClueFlow *flow,
			       HTMLEngine *engine,
			       guint8 indentation)
{
	g_return_if_fail (flow != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	if (flow->level == indentation)
		return;

	flow->level = indentation;

	relayout_with_siblings (flow, engine);
}

guint8
html_clueflow_get_indentation (HTMLClueFlow *flow)
{
	g_return_val_if_fail (flow != NULL, 0);

	return flow->level;
}

void
html_clueflow_set_properties (HTMLClueFlow *flow,
			      HTMLEngine *engine,
			      HTMLClueFlowStyle style,
			      guint8 indentation,
			      HTMLHAlignType alignment)
{
	g_return_if_fail (flow != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	HTML_CLUE (flow)->halign = alignment;

	flow->style = style;
	flow->level = indentation;

	relayout_and_draw (HTML_OBJECT (flow), engine);
}

void
html_clueflow_get_properties (HTMLClueFlow *flow,
			      HTMLClueFlowStyle *style_return,
			      guint8 *indentation_return,
			      HTMLHAlignType *alignment_return)
{
	g_return_if_fail (flow != NULL);

	if (style_return != NULL)
		*style_return = flow->style;
	if (indentation_return != NULL)
		*indentation_return = flow->level;
	if (alignment_return != NULL)
		*alignment_return = HTML_CLUE (flow)->halign;
}


void
html_clueflow_remove_text_slaves (HTMLClueFlow *flow)
{
	HTMLClue *clue;
	HTMLObject *p;
	HTMLObject *pnext;

	g_return_if_fail (flow != NULL);

	clue = HTML_CLUE (flow);
	for (p = clue->head; p != NULL; p = pnext) {
		pnext = p->next;

		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE) {
			html_clue_remove (clue, p);
			html_object_destroy (p);
		}
	}
}

#ifdef GTKHTML_HAVE_PSPELL

/* spell checking */

#include "htmlinterval.h"

static guint
get_text_len (HTMLClue *clue, HTMLInterval *i)
{
	HTMLObject *obj;
	guint len;

	g_assert (i);
	g_assert (i->from);
	g_assert (i->to);

	len = 0;
	obj = i->from;
	while (1) {
		len += html_object_get_length (obj);
		if (obj == i->to) break;
		obj = obj->next;
		g_assert (obj);
	}
	/* correct len by taking care of interval offsets */
	len -= i->from_offset + html_object_get_length (obj) - i->to_offset;

	return len;
}

static gchar *
get_text (HTMLClue *clue, HTMLInterval *i)
{
	HTMLObject *obj;
	guint cl, len = 0;
	gchar *text, *ct;

	len = get_text_len (clue, i);
	ct  = text = g_malloc (len+1);
	text [len] = 0;

	obj = i->from;
	while (obj) {
		cl = html_interval_get_length (i, obj);
		if (html_object_is_text (obj))
			strncpy (ct, HTML_TEXT (obj)->text + html_interval_get_start (i, obj), cl);
		else
			if (cl == 1) *ct = ' ';
			else memset (ct, ' ', cl);
		ct += cl;
		if (obj == i->to) break;
		obj = obj->next;
	}

	/* printf ("get_text: \"%s\"\n", text); */

	return text;
}

static HTMLObject *
next_obj_and_clear (HTMLObject *obj, guint *off, gboolean *is_text, HTMLInterval *i)
{
	*off += html_object_get_length (obj) - html_interval_get_start (i, obj);
	obj = obj->next;
	if (obj && (*is_text = html_object_is_text (obj)))
		html_text_spell_errors_clear_interval (HTML_TEXT (obj), i);

	return obj;
}

static HTMLObject *
spell_check_word_mark (HTMLObject *obj, const gchar *text, const gchar *word, guint *off, HTMLInterval *i)
{
	guint w_off, ioff;
	guint len = strlen (word);
	gboolean is_text;

	/* printf ("[not in dictionary word off: %d off: %d]\n", word - text, *off); */
	is_text = html_object_is_text (obj);
	w_off   = word - text;
	while (obj && (!is_text || (is_text && *off + HTML_TEXT (obj)->text_len <= w_off)))
		obj = next_obj_and_clear (obj, off, &is_text, i);

	/* printf ("is_text: %d len: %d obj: %p off: %d\n", is_text, len, obj, *off); */
	if (obj && is_text) {
		guint tlen;
		guint toff;

		while (len) {
			toff  = w_off - *off;
			ioff  = html_interval_get_start (i, obj);
			tlen  = MIN (HTML_TEXT (obj)->text_len - toff - ioff, len);
			g_assert (!strncmp (text + w_off, HTML_TEXT (obj)->text + toff + ioff, tlen));
			/* printf ("add spell error - word: %s off: %d beg: %s len: %d\n",
			   word, *off, HTML_TEXT (obj)->text + toff, tlen); */
			html_text_spell_errors_add (HTML_TEXT (obj),
						    ioff + toff, tlen);
			len   -= tlen;
			w_off += tlen;
			if (len)
				do obj = next_obj_and_clear (obj, off, &is_text, i); while (obj && !is_text);
			/* printf ("off: %d\n", *off); */
			g_assert (!len || obj);
		}
	}

	return obj;
}

void
html_clueflow_spell_check (HTMLClueFlow *flow, HTMLEngine *e, HTMLInterval *i)
{
	HTMLObject *obj;
	HTMLClue *clue;
	guint off;
	gchar *text, *ct, *word;

	g_assert (HTML_OBJECT_TYPE (flow) == HTML_TYPE_CLUEFLOW);

	/* printf ("html_clueflow_spell_check\n"); */

	if (!e->spell_checker)
		return;

	off  = 0;
	clue = HTML_CLUE (flow);
	if (!i)
		i = html_interval_new (clue->head, clue->tail, 0, html_object_get_length (clue->tail));
	text = get_text (clue, i);
	obj  = i->from;
	if (obj && html_object_is_text (obj))
		html_text_spell_errors_clear_interval (HTML_TEXT (obj), i);

	if (text) {
		ct = text;
		while (*ct) {
			/* find begin of word */
			while (*ct && !html_is_in_word (*ct)) ct++;
			word = ct;
			/* find end of word */
			while (html_is_in_word (*ct)) ct++;

			/* test if we have found word */
			if (word != ct) {
				gint result;
				gchar bak;

				bak = *ct;
				*ct = 0;
				/* printf ("off %d going to test word: \"%s\"\n", off, word); */
				result = pspell_manager_check (e->spell_checker, word);

				if (result == 1) {
					gboolean is_text = (obj) ? html_object_is_text (obj) : FALSE;
					while (obj && (!is_text || (is_text && off + HTML_TEXT (obj)->text_len < ct - text)))
						obj = next_obj_and_clear (obj, &off, &is_text, i);
				} else {
					if (result == 0) {
						if (obj)
							obj = spell_check_word_mark (obj, text, word, &off, i);
					} else if (result == -1)
						g_warning ("pspell error: %s\n",
							   pspell_manager_error_message (e->spell_checker));
					else
						g_assert_not_reached ();
				}

				*ct = bak;
				if (*ct) ct++;
			}
		}
	}
}

#endif /* GTKHTML_HAVE_PSPELL */

