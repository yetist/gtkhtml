/* 
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
#include "htmlcluealigned.h"
#include "htmlcluev.h"
#include "htmlobject.h"

void html_cluev_append_right_aligned (HTMLClue *clue, HTMLClue *aclue);

static HTMLObject *
cluev_next_aligned (HTMLObject *aclue)
{
	return HTML_OBJECT (HTML_CLUEALIGNED (aclue)->nextAligned);
}

void
html_cluev_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLObject *aclue;

	html_clue_draw (o, p, x, y ,width, height, tx, ty);

	/* print aligned objects */
	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;
	
	/* FIXME: left aligned objects too */

	for (aclue = HTML_CLUEV (o)->alignRightList; aclue != 0; aclue = cluev_next_aligned (aclue)) {
		aclue->draw (aclue, p, 
			     tx + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->x, 
			     ty + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y - HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent,
			     width, height, tx, ty);
	}
}

void
html_cluev_set_max_width (HTMLObject *o, gint max_width)
{
	HTMLObject *obj;
	
	if (!(o->flags & FixedWidth)) {
		o->max_width = max_width;
		if (o->percent > 0)
			o->width = max_width * o->percent / 100;
		else 
			o->width = o->max_width;
	}

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj)
		obj->set_max_width (obj, o->width);

}

gint
html_cluev_get_left_margin (HTMLClue *clue, gint y)
{
	HTMLClueV *cluev = HTML_CLUEV (clue);
	gint margin = 0;
	HTMLObject *aclue;
	
	for (aclue = cluev->alignLeftList; aclue != 0; aclue = cluev_next_aligned (aclue)) {
		if (aclue->y - aclue->ascent + 
		    HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y -
		    HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent <= y &&
		    aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y - 
		    HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent > y)
			margin = aclue->x + aclue->width;
	}

	return margin;
}

gint
html_cluev_get_right_margin (HTMLClue *o, gint y)
{
	HTMLClueV *cluev = HTML_CLUEV (o);
	gint margin = HTML_OBJECT (o)->max_width;
	/* FIXME: Should be HTMLAligned */
	HTMLObject *aclue;
	
	for (aclue = cluev->alignRightList; aclue != 0; aclue = cluev_next_aligned (aclue)) {
		if (aclue->y - aclue->ascent + 
		    HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y -
		    HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent <= y &&
		    aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent > y)
			margin = aclue->x;
	}

	return margin;
}

void
html_cluev_calc_size (HTMLObject *o, HTMLObject *parent)
{
	HTMLClueV *cluev = HTML_CLUEV (o);
	HTMLClue *clue = HTML_CLUE (o);

	HTMLObject *obj;
	HTMLObject *aclue;

	gint lmargin = parent ? HTML_CLUE (parent)->get_left_margin (HTML_CLUE (parent), o->y) + cluev->padding :
	  cluev->padding;

	/* If we have already called calc_size for the children, then just
	   continue from the last object done in previous call. */
	
	if (clue->curr) {
		o->ascent = cluev->padding;
		
		/* Get the current ascent not including curr */
		obj = clue->head;
		while (obj != clue->curr) {
			o->ascent += obj->ascent + obj->descent;
			obj = obj->nextObj;
		}

		/* FIXME: Remove any aligned objects previously added by the current
		   object */
	}
	else {
		o->ascent = cluev->padding;
		o->descent = 0;
		clue->curr = clue->head;
	}

	while (clue->curr != 0) {
		/* Set an initial ypos so that the alignment stuff knows where
		   the top of this object is */
		clue->curr->y = o->ascent;
		clue->curr->calc_size (clue->curr, o);
		if (clue->curr->width > o->width - (cluev->padding << 1))
			o->width = clue->curr->width + (cluev->padding << 1);
		o->ascent += clue->curr->ascent + clue->curr->descent;
		clue->curr->x = lmargin;
		clue->curr->y = o->ascent - clue->curr->descent;
		clue->curr = clue->curr->nextObj;
	}

	o->ascent += cluev->padding;

	/* Remember the last object so that we can start from here next time
	   we are called. */
	clue->curr = clue->tail;

	if ((o->max_width != 0) && (o->width > o->max_width))
		o->width = o->max_width;
	
	if (clue->halign == HCenter) {
		for (obj = clue->head; obj != 0; obj = obj->nextObj)
			obj->x = lmargin + (o->width - obj->width) / 2;
	}
	else if (clue->halign == Right) {
		for (obj = clue->head; obj != 0; obj = obj->nextObj)
			obj->x = lmargin + (o->width - obj->width);
	}
	
	for (aclue = cluev->alignLeftList; aclue != 0; aclue = cluev_next_aligned (aclue)) {
		if (aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y - 
		    HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent > o->ascent)
			o->ascent = aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y -
				HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent;
	}
	for (aclue = cluev->alignRightList; aclue != 0; aclue = cluev_next_aligned (aclue)) {
		if (aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y -
		    HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent > o->ascent)
			o->ascent = aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y -
				HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent;
	}
}

void
html_cluev_find_free_area (HTMLClue *clue, gint y, gint width, gint height,
			   gint indent, gint *y_pos, gint *_lmargin, gint *_rmargin)
{
	HTMLClueV *cluev = HTML_CLUEV (clue);
	gint try_y;
	gint lmargin;
	gint rmargin;
	gint lm, rm;
	HTMLObject *aclue;
	gint next_y, top_y, base_y=0;

	try_y = y;

	while (1) {
		lmargin = indent;
		rmargin = HTML_OBJECT (clue)->max_width;
		next_y = 0;
		
		for (aclue = cluev->alignLeftList; aclue != 0; aclue = cluev_next_aligned (aclue)) {
			base_y = aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y - HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent;
			top_y = base_y - aclue->ascent;

			if ((top_y <= try_y + height) && (base_y > try_y)) {
				lm = aclue->x + aclue->width;
				if (lm > lmargin)
					lmargin = lm;
				
				if ((next_y == 0) || (base_y < next_y)) {
					next_y = base_y;

				}
			}
		}

		for (aclue = cluev->alignRightList; aclue != 0; aclue = cluev_next_aligned (aclue)) {
			base_y = aclue->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y - 
				HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->ascent;
			top_y = base_y - aclue->ascent;

			if ((top_y <= try_y + height) && (base_y > try_y)) {
				rm = aclue->x;
				if (rm < rmargin)
					rmargin = rm;
				
				if ((next_y == 0) || (base_y < next_y)) {
					next_y = base_y;
				}
			}
		}
		
		if ((lmargin == indent) && (rmargin == HTML_OBJECT (clue)->max_width))
			break;
		
		if ((rmargin - lmargin) >= width)
			break;

		try_y = next_y;
	}
	
	*y_pos = try_y;
	*_rmargin = rmargin;
	*_lmargin = lmargin;
}

HTMLObject *
html_cluev_new (int x, int y, int max_width, int percent)
{
	HTMLClueV *cluev = g_new0 (HTMLClueV, 1);
	HTMLObject *object = HTML_OBJECT (cluev);
	HTMLClue *clue = HTML_CLUE (cluev);
	html_clue_init (clue, ClueV);

	/* HTMLClue functions */
	clue->get_left_margin = html_cluev_get_left_margin;
	clue->get_right_margin = html_cluev_get_right_margin;
	clue->find_free_area = html_cluev_find_free_area;
	clue->appended = html_cluev_appended;
	clue->append_right_aligned = html_cluev_append_right_aligned;

	/* HTMLObject functions */
	object->calc_size = html_cluev_calc_size;
	object->set_max_width = html_cluev_set_max_width;
	object->reset = html_cluev_reset;
	object->draw = html_cluev_draw;

	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->percent = percent;
	clue->valign = Bottom;
	clue->halign = Left;
	clue->head = clue->tail = clue->curr = 0;

	if (object->percent > 0) {
		object->width = max_width * percent / 100;
		object->flags &= ~FixedWidth;
	}
	else if (percent < 0) {
		object->width = max_width;
		object->flags &= ~FixedWidth;
	}
	else
		object->width = max_width;

	cluev->alignLeftList = 0;
	cluev->alignRightList = 0;
	cluev->padding = 0;
	
	return object;
}

void
html_cluev_reset (HTMLObject *clue)
{
	HTMLClueV *cluev = HTML_CLUEV (clue);

	html_clue_reset (clue);

	cluev->alignLeftList = 0;
	cluev->alignRightList = 0;
}

gboolean
html_cluev_appended (HTMLClue *clue, HTMLClue *aclue)
{
	/* Returns whether aclue is already in the alignList */
	HTMLClueAligned *aligned;
	
	if (aclue->halign == Left) {
		g_print ("FIIIIXMEEEE\n");
		aligned = NULL;
	}
	else {
		aligned = HTML_CLUEALIGNED (HTML_CLUEV (clue)->alignRightList);
	}

	while (aligned) {
		if (aligned == HTML_CLUEALIGNED (aclue))
			return TRUE;
		HTML_OBJECT (aligned) = HTML_OBJECT (aligned)->nextObj;
	}
	return FALSE;
}

void
html_cluev_append_right_aligned (HTMLClue *clue, HTMLClue *aclue)
{
	gint y_pos;
	gint start_y = 0;
	gint lmargin;
	gint rmargin;
	HTMLClueAligned *aligned;

	aligned = HTML_CLUEALIGNED (HTML_CLUEV (clue)->alignRightList);
	if (aligned) {
		while (aligned->nextAligned) {
			aligned = aligned->nextAligned;
		}
		y_pos = HTML_OBJECT (aligned)->y + HTML_OBJECT (aligned->prnt)->y;
		if (y_pos > start_y)
			start_y = y_pos;
	}

	y_pos = HTML_OBJECT (aclue)->y + HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y;
	if (y_pos > start_y)
		start_y = y_pos;
	
	/* Start looking for space from the position of the last object in
	   the left-aligned list on, or from the current position of the
	   object. */
	html_cluev_find_free_area (clue, start_y - HTML_OBJECT (aclue)->ascent,
				   HTML_OBJECT (aclue)->width, 
				   HTML_OBJECT (aclue)->ascent + 
				   HTML_OBJECT (aclue)->descent, 0,
				   &y_pos, &lmargin, &rmargin);

	/* Set position */
	HTML_OBJECT (aclue)->x = rmargin - HTML_OBJECT (aclue)->width;
	HTML_OBJECT (aclue)->y = y_pos - HTML_OBJECT (HTML_CLUEALIGNED (aclue)->prnt)->y + HTML_OBJECT (aclue)->ascent;
	
	/* Insert clue in align list */
	if (!HTML_CLUEV (clue)->alignRightList) {
		HTML_CLUEV (clue)->alignRightList = HTML_OBJECT (aclue);
		HTML_CLUEALIGNED (aclue)->nextAligned = NULL;
	}
	else {
		HTMLClueAligned *obj = HTML_CLUEALIGNED (HTML_CLUEV (clue)->alignRightList);
		while (obj->nextAligned) {
			if (obj == HTML_CLUEALIGNED (aclue))
				return;
			obj = obj->nextAligned;
		}
		if (obj == HTML_CLUEALIGNED (aclue))
			return;

		obj->nextAligned = HTML_CLUEALIGNED (aclue);
		HTML_CLUEALIGNED (aclue)->nextAligned = NULL;
	}
}

