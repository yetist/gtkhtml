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

#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluealigned.h"
#include "htmltext.h"
#include "htmlvspace.h"


HTMLClueFlowClass html_clueflow_class;

#define HCF_CLASS(x) HTML_CLUEFLOW_CLASS (HTML_OBJECT (x)->klass)


static guint
calc_padding (HTMLPainter *painter)
{
	guint ascent, descent;

	/* FIXME maybe this should depend on the style.  */
	ascent = html_painter_calc_ascent (painter, HTML_FONT_STYLE_SIZE_3);
	descent = html_painter_calc_descent (painter, HTML_FONT_STYLE_SIZE_3);

	return ascent + descent;
}

static guint
calc_indent_unit (HTMLPainter *painter)
{
	guint ascent, descent;

	ascent = html_painter_calc_ascent (painter, HTML_FONT_STYLE_SIZE_3);
	descent = html_painter_calc_descent (painter, HTML_FONT_STYLE_SIZE_3);

	return (ascent + descent) * 3;
}

static guint
calc_bullet_size (HTMLPainter *painter)
{
	guint ascent, descent;

	ascent = html_painter_calc_ascent (painter, HTML_FONT_STYLE_SIZE_3);
	descent = html_painter_calc_descent (painter, HTML_FONT_STYLE_SIZE_3);

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

static void
add_pre_padding (HTMLClueFlow *flow,
		 guint pad)
{
	HTMLObject *prev_object;

	prev_object = HTML_OBJECT (flow)->prev;
	if (prev_object == NULL)
		return;

	if (HTML_OBJECT_TYPE (prev_object) == HTML_TYPE_CLUEFLOW) {
		HTMLClueFlow *prev;

		prev = HTML_CLUEFLOW (prev_object);
		if (prev->list_level > 0 && flow->list_level == 0) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		if (prev->quote_level != flow->quote_level) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		if (flow->style == HTML_CLUEFLOW_STYLE_PRE
		    && prev->style != HTML_CLUEFLOW_STYLE_PRE
		    && ! is_header (prev)) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		if (is_header (flow) && ! is_header (prev)) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		return;
	}

	if (is_header (flow) || flow->quote_level > 0 || flow->list_level > 0)
		HTML_OBJECT (flow)->ascent += pad;
}

static void
add_post_padding (HTMLClueFlow *flow,
		  guint pad)
{
	HTMLObject *next_object;

	next_object = HTML_OBJECT (flow)->next;
	if (next_object == NULL)
		return;

	if (HTML_OBJECT_TYPE (next_object) == HTML_TYPE_CLUEFLOW) {
		HTMLClueFlow *next;

		next = HTML_CLUEFLOW (next_object);
		if (next->list_level > 0 && flow->list_level == 0) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		if (next->quote_level != flow->quote_level) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		if (flow->style == HTML_CLUEFLOW_STYLE_PRE
		    && next->style != HTML_CLUEFLOW_STYLE_PRE
		    && ! is_header (next)) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		if (is_header (flow)) {
			HTML_OBJECT (flow)->ascent += pad;
			return;
		}

		return;
	}

	if (is_header (flow) || flow->quote_level > 0 || flow->list_level > 0)
		HTML_OBJECT (flow)->ascent += pad;
}

static guint
get_indent (HTMLClueFlow *flow,
	    HTMLPainter *painter)
{
	guint total_level;
	guint indent;

	total_level = flow->quote_level + flow->list_level;

	if (total_level > 0 || ! is_item (flow))
		indent = total_level * calc_indent_unit (painter);
	else
		indent = 2 * calc_bullet_size (painter);

	return indent;
}

static const gchar *
get_tag_for_style (const HTMLClueFlow *flow)
{
	switch (flow->style) {
	case HTML_CLUEFLOW_STYLE_NORMAL:
		return NULL;
	case HTML_CLUEFLOW_STYLE_H1:
		return "H1";
	case HTML_CLUEFLOW_STYLE_H2:
		return "H2";
	case HTML_CLUEFLOW_STYLE_H3:
		return "H3";
	case HTML_CLUEFLOW_STYLE_H4:
		return "H4";
	case HTML_CLUEFLOW_STYLE_H5:
		return "H5";
	case HTML_CLUEFLOW_STYLE_H6:
		return "H6";
	case HTML_CLUEFLOW_STYLE_ADDRESS:
		return "ADDRESS";
	case HTML_CLUEFLOW_STYLE_PRE:
		return "PRE";
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return "LI";
	default:
		g_warning ("Unknown HTMLClueFlowStyle %d", flow->style);
		return NULL;
	}
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

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next)
		html_object_set_max_width (obj, painter, o->max_width - indent);
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	HTMLObject *obj;
	gint w;
	gint tempMinWidth;
	gboolean is_pre;

	tempMinWidth = 0;
	w = 0;
	is_pre = (HTML_CLUEFLOW (o)->style == HTML_CLUEFLOW_STYLE_PRE);

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		w += html_object_calc_min_width (obj, painter);

		if (is_pre
		    && ! (obj->flags & HTML_OBJECT_FLAG_NEWLINE)
		    && obj->next != NULL)
			continue;

		if (w > tempMinWidth)
			tempMinWidth = w;

		w = 0;
	}

	tempMinWidth += get_indent (HTML_CLUEFLOW (o), painter);

	return tempMinWidth;
}

/* EP CHECK: should be mostly OK.  */
/* FIXME: But it's awful.  Too big and ugly.  */
static void
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
	gboolean is_pre;
	gint lmargin, rmargin;
	gint indent;
	gint oldy;
	gint w, a, d;
	guint padding;

	clue = HTML_CLUE (o);

	obj = clue->head;
	line = clue->head;
	clear = HTML_CLEAR_NONE;
	indent = get_indent (HTML_CLUEFLOW (o), painter);

	if (HTML_CLUEFLOW (o)->style == HTML_CLUEFLOW_STYLE_PRE)
		is_pre = TRUE;
	else
		is_pre = FALSE;

	o->ascent = 0;
	o->descent = 0;
	o->width = 0;

	padding = calc_padding (painter);

	add_pre_padding (HTML_CLUEFLOW (o), padding);

	lmargin = html_clue_get_left_margin (HTML_CLUE (o->parent), o->y);
	if (indent > lmargin)
		lmargin = indent;
	rmargin = html_clue_get_right_margin (HTML_CLUE (o->parent), o->y);

	w = lmargin;
	a = 0;
	d = 0;
	newLine = FALSE;
	firstLine = TRUE;

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
			obj->x = w;
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
					obj->x = lmargin;
					obj->y = o->ascent + obj->ascent;

					html_clue_append_left_aligned (HTML_CLUE (o->parent),
								       HTML_CLUE (c));
				} else {
					obj->x = rmargin - obj->width;
					obj->y = o->ascent + obj->ascent;
					
					html_clue_append_right_aligned (HTML_CLUE (o->parent),
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
				gint width_left;
			  
				run->max_width = rmargin - lmargin;

				if (is_pre)
					width_left = -1;
				else
					width_left = rmargin - runWidth - w;

				fit = html_object_fit_line (run,
							    painter,
							    w + runWidth == lmargin,
							    obj == line,
							    width_left);
				if (fit == HTML_FIT_NONE) {
					newLine = TRUE;
					break;
				}
				
				html_object_calc_size (run, painter);
				runWidth += run->width;

				/* If this run cannot fit in the allowed area, break it
				   on a non-separator */
				if (! is_pre && run != obj && runWidth > rmargin - lmargin)
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

				if (! is_pre && runWidth > rmargin - lmargin)
					break;
			}

			/* If these objects do not fit in the current line and we are
			   not at the start of a line then end the current line in
			   preparation to add this run in the next pass. */
			if (! is_pre && w > lmargin && w + runWidth > rmargin)
				newLine = TRUE;

			if (! newLine) {
				gint new_y, new_lmargin, new_rmargin;

				/* Check if the run fits in the current flow area 
				   especially with respect to its height.
				   If not, find a rectangle with height a+b. The size of
				   the rectangle will be rmargin-lmargin. */

				html_clue_find_free_area (HTML_CLUE (o->parent),
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
					html_object_set_max_ascent (line, painter, a);
					html_object_set_max_descent (line, painter, d);
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

			lmargin = html_clue_get_left_margin (HTML_CLUE (o->parent), o->y);
			if (indent > lmargin)
				lmargin = indent;
			rmargin = html_clue_get_right_margin (HTML_CLUE (o->parent), o->y);

			w = lmargin;
			d = 0;
			a = 0;
			
			newLine = FALSE;
			clear = HTML_CLEAR_NONE;
		}
	}
	
	if (o->width < o->max_width)
		o->width = o->max_width;

	add_post_padding (HTML_CLUEFLOW (o), padding);
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
		} else {
			if (w > maxw)
				maxw = w;
			w = 0;
		}
	}
	
	if (w > maxw)
		maxw = w;

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

		html_painter_set_pen (painter, html_painter_get_black (painter));

		if (clueflow->list_level == 0 || (clueflow->list_level & 1) != 0)
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

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	HTMLClueFlow *clueflow;
	const gchar *tag;

	clueflow = HTML_CLUEFLOW (self);

	tag = get_tag_for_style (clueflow);

	/* FIXME: Indentation must be handled specially.  */

	/* Start tag.  */
	if (tag != NULL
	    && (! html_engine_save_output_string (state, "<")
		|| ! html_engine_save_output_string (state, tag)
		|| ! html_engine_save_output_string (state, ">")))
		return FALSE;

	/* Paragraph's content.  */
	if (! HTML_OBJECT_CLASS (&html_clue_class)->save (self, state))
		return FALSE;

	/* End tag.  */
	if (tag != NULL) {
		if (! html_engine_save_output_string (state, "</")
		    || ! html_engine_save_output_string (state, tag)
		    || ! html_engine_save_output_string (state, ">\n"))
			return FALSE;
	} else if (HTML_OBJECT_TYPE (HTML_CLUE (self)->tail) != HTML_TYPE_RULE) {
		/* This comparison is a nasty hack: the rule takes all of the
                   space, so we don't want it to create a new newline if it's
                   the last element on the paragraph.  */
		html_engine_save_output_string (state, "<BR>\n");
	}

	return TRUE;
}


static HTMLFontStyle
get_default_font_style (const HTMLClueFlow *self)
{
	switch (self->style) {
	case HTML_CLUEFLOW_STYLE_NORMAL:
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return HTML_FONT_STYLE_SIZE_3;
	case HTML_CLUEFLOW_STYLE_ADDRESS:
		return HTML_FONT_STYLE_SIZE_3 | HTML_FONT_STYLE_ITALIC;
	case HTML_CLUEFLOW_STYLE_PRE:
		return HTML_FONT_STYLE_SIZE_3 | HTML_FONT_STYLE_FIXED;
	case HTML_CLUEFLOW_STYLE_H1:
		return HTML_FONT_STYLE_SIZE_6 | HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H2:
		return HTML_FONT_STYLE_SIZE_5 | HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H3:
		return HTML_FONT_STYLE_SIZE_4 | HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H4:
		return HTML_FONT_STYLE_SIZE_3 | HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H5:
		return HTML_FONT_STYLE_SIZE_2 | HTML_FONT_STYLE_BOLD;
	case HTML_CLUEFLOW_STYLE_H6:
		return HTML_FONT_STYLE_SIZE_1 | HTML_FONT_STYLE_BOLD;
	default:
		g_warning ("Unexpected HTMLClueFlow style %d", self->style);
		return HTML_FONT_STYLE_DEFAULT;
	}
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
	object_class->save = save;

	klass->get_default_font_style = get_default_font_style;
}

void
html_clueflow_init (HTMLClueFlow *clueflow,
		    HTMLClueFlowClass *klass,
		    HTMLClueFlowStyle style,
		    guint8 list_level,
		    guint8 quote_level)
{
	HTMLObject *object;
	HTMLClue *clue;

	object = HTML_OBJECT (clueflow);
	clue = HTML_CLUE (clueflow);

	html_clue_init (clue, HTML_CLUE_CLASS (klass));

	object->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;

	clue->valign = HTML_VALIGN_BOTTOM;
	clue->halign = HTML_HALIGN_LEFT;

	clueflow->style = style;

	clueflow->list_level = list_level;
	clueflow->quote_level = quote_level; 
}

HTMLObject *
html_clueflow_new (HTMLClueFlowStyle style,
		   guint8 list_level,
		   guint8 quote_level)
{
	HTMLClueFlow *clueflow;

	clueflow = g_new (HTMLClueFlow, 1);
	html_clueflow_init (clueflow, &html_clueflow_class, style,
			    list_level, quote_level);

	return HTML_OBJECT (clueflow);
}


/* Virtual methods.  */

HTMLFontStyle
html_clueflow_get_default_font_style (const HTMLClueFlow *self)
{
	g_return_val_if_fail (self != NULL, HTML_FONT_STYLE_DEFAULT);

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

	new = HTML_CLUEFLOW (html_clueflow_new (clue->style,
						clue->list_level,
						clue->quote_level));

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

	/* Put the children into the new clue.  */

	html_clue_append (HTML_CLUE (new), child);

	/* Return the new clue.  */

	return new;
}
