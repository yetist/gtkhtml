/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
	      (C) 1999 Anders Carlsson (andersca@gnu.org)

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
#include <glib.h>
#include "htmlcluev.h"
#include "htmltablecell.h"
#include "htmlobject.h"


HTMLTableCellClass html_table_cell_class;


/* HTMLObject methods.  */

static gint
calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint minWidth = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		gint w = html_object_calc_min_width (obj);
		if (w > minWidth)
			minWidth = w;
	}

	if ((o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH)) {

		/* Our minimum width is at least our fixed width */
		if (o->max_width > minWidth)
			minWidth = o->max_width;

		/* And our actual width is at least our minimum width */
		if (o->width < minWidth)
			o->width = minWidth;
	}

	return minWidth;
}

static void
set_max_width (HTMLObject *o, gint max_width)
{
	HTMLObject *obj;

	o->max_width = max_width;

	if (max_width > 0) {
		if (o->percent > 0)
			o->width = max_width * o->percent / 100;
		else if (!(o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH))
			o->width = max_width;
	}

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		html_object_set_max_width (obj, max_width);
	}
}

static void
draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, 
      gint width, gint height, gint tx, gint ty)
{
	HTMLTableCell *cell = HTML_TABLE_CELL (o);
	gint top, bottom;

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	top = y - (o->y - o->ascent);
	bottom = top + height;
	if (top < -cell->padding)
		top = -cell->padding;
	if (bottom > o->ascent + cell->padding)
		bottom = o->ascent + cell->padding;
	html_painter_set_pen (p, &cell->bg);
	html_painter_fill_rect (p, tx + o->x - cell->padding,
				ty + o->y - o->ascent + top,
				o->width + cell->padding * 2,
				bottom - top);

	(* HTML_OBJECT_CLASS (&html_cluev_class)->draw) (o, p, x, y,
							 width, height, tx, ty);
}


void
html_table_cell_type_init (void)
{
	html_table_cell_class_init (&html_table_cell_class,
				    HTML_TYPE_TABLECELL);
}

void
html_table_cell_class_init (HTMLTableCellClass *klass,
			    HTMLType type)
{
	HTMLObjectClass *object_class;
	HTMLClueVClass *cluev_class;

	object_class = HTML_OBJECT_CLASS (klass);
	cluev_class = HTML_CLUEV_CLASS (klass);

	html_cluev_class_init (cluev_class, type);

	/* FIXME destroy */

	object_class->calc_min_width = calc_min_width;
	object_class->set_max_width = set_max_width;
	object_class->draw = draw;
}

void
html_table_cell_init (HTMLTableCell *cell,
		      HTMLTableCellClass *klass,
		      gint x, gint y,
		      gint max_width,
		      gint percent,
		      gint rs, gint cs,
		      gint pad)
{
	HTMLObject *object;
	HTMLClueV *cluev;
	HTMLClue *clue;

	object = HTML_OBJECT (cell);
	cluev = HTML_CLUEV (cell);
	clue = HTML_CLUE (cell);

	html_cluev_init (cluev, HTML_CLUEV_CLASS (klass),
			 x, y, max_width, percent);

	if (percent > 0) 
		object->width = max_width * percent / 100;
	else if (percent < 0) 
		object->width = max_width;
	
	object->flags &= ~HTML_OBJECT_FLAG_FIXEDWIDTH;

	clue->valign = Bottom;
	clue->halign = Left;

	cell->padding = pad;
	cell->refcount = 0;
	cell->rspan = rs;
	cell->cspan = cs;
}

HTMLObject *
html_table_cell_new (gint x, gint y, gint max_width, gint percent,
		     gint rs, gint cs, gint pad)
{
	HTMLTableCell *cell;

	cell = g_new (HTMLTableCell, 1);
	html_table_cell_init (cell, &html_table_cell_class,
			      x, y, max_width, percent, rs, cs, pad);

	return HTML_OBJECT (cell);
}

void
html_table_cell_link (HTMLTableCell *cell)
{
	cell->refcount++;
}

void
html_table_cell_unlink (HTMLTableCell *cell)
{
	cell->refcount--;

	/* AAAAAAAAARGH!  WRONG!  FIXME!  */
	if (cell->refcount == 0)
		g_free (cell);
}

void
html_table_cell_set_width (HTMLTableCell *cell, gint width)
{
	HTMLObject *obj;
	HTMLObject *o = HTML_OBJECT (cell);

	o->width = width;
	if (!(o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH))
	    o->max_width = width;

	for (obj = HTML_CLUE (cell)->head; obj != 0; obj = obj->next)
		html_object_set_max_width (obj, width);
}
