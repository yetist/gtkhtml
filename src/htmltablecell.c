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
#include <glib.h>
#include "htmlcluev.h"
#include "htmltablecell.h"
#include "htmlobject.h"

static gint html_table_cell_calc_min_width (HTMLObject *o);
static void html_table_cell_set_max_width (HTMLObject *o, gint max_width);
static void html_table_cell_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);
HTMLObject *
html_table_cell_new (gint x, gint y, gint max_width, gint percent,
		     gint rs, gint cs, gint pad)
{
	HTMLTableCell *cell = g_new0 (HTMLTableCell, 1);
	HTMLObject *object = HTML_OBJECT (cell);
	HTMLClue *clue = HTML_CLUE (cell);
	html_object_init (object, TableCell);

	/* HTMLObject functions */
	object->calc_min_width = html_table_cell_calc_min_width;
	object->calc_size = html_cluev_calc_size;
	object->set_max_ascent = html_clue_set_max_ascent;
	object->set_max_width = html_table_cell_set_max_width;
	object->calc_preferred_width = html_clue_calc_preferred_width;

	/* FIXME: fix backgrounds */
	object->draw = html_table_cell_draw;

	/* HTMLClue functions */
	clue->get_left_margin = html_cluev_get_left_margin;
	clue->get_right_margin = html_cluev_get_right_margin;
	clue->find_free_area = html_cluev_find_free_area;
	clue->appended = html_cluev_appended;
	clue->append_right_aligned = html_cluev_append_right_aligned;

	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->width = max_width;
	object->percent = percent;
	clue->valign = Bottom;
	clue->halign = Left;
	cell->padding = pad;
	cell->refcount = 0;
	cell->rspan = rs;
	cell->cspan = cs;
	
	if (percent > 0) 
		object->width = max_width * percent / 100;
	else if (percent < 0) 
		object->width = max_width;
	
	object->flags &= ~FixedWidth;

	return object;
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

	if (cell->refcount == 0)
		g_free (cell);
}

static gint
html_table_cell_calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint minWidth = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		gint w = obj->calc_min_width (o);
		if (w > minWidth)
			minWidth = w;
	}

	if ((o->flags & FixedWidth)) {

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
html_table_cell_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, 
		      gint width, gint height, gint tx, gint ty)
{
	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;
	
	html_cluev_draw (o, p, x, y, width, height, tx, ty);
}

static void
html_table_cell_set_max_width (HTMLObject *o, gint max_width)
{
	HTMLObject *obj;

	o->max_width = max_width;

	if (max_width > 0) {
		if (o->percent > 0)
			o->width = max_width * o->percent / 100;
		else if (!(o->flags & FixedWidth))
			o->width = max_width;
	}

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		obj->set_max_width (obj, max_width);
	}
}

void
html_table_cell_set_width (HTMLTableCell *cell, gint width)
{
	HTMLObject *obj;
	HTMLObject *o = HTML_OBJECT (cell);

	o->width = width;
	if (!(o->flags & FixedWidth))
	    o->max_width = width;

	g_print ("The width being set: %d\n", width);
	for (obj = HTML_CLUE (cell)->head; obj != 0; obj = obj->nextObj)
		obj->set_max_width (obj, width);
}












