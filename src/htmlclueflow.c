#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluealigned.h"
#include "htmlvspace.h"

static void html_clueflow_calc_size (HTMLObject *clue, HTMLObject *parent);
static gint html_clueflow_calc_min_width (HTMLObject *o);
static gint html_clueflow_calc_preferred_width (HTMLObject *o);

HTMLObject *
html_clueflow_new (int x, int y, int max_width, gint percent)
{
	HTMLClueFlow *clueflow = g_new0 (HTMLClueFlow, 1);
	HTMLObject *object = HTML_OBJECT (clueflow);
	HTMLClue *clue = HTML_CLUE (clueflow);
	html_clue_init (clue, ClueFlow);

	/* HTMLObject functions */
	object->calc_size = html_clueflow_calc_size;
	object->set_max_width = html_clueflow_set_max_width;
	object->calc_min_width = html_clueflow_calc_min_width;
	object->calc_preferred_width = html_clueflow_calc_preferred_width;

	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->percent = percent;

	clue->valign = Bottom;
	clue->halign = Left;
	clue->head = clue->tail = clue->curr = 0;
	object->width = max_width;
	object->flags &= ~FixedWidth;

	return object;
}

void
html_clueflow_set_max_width (HTMLObject *o, gint max_width)
{
	HTMLObject *obj;

	o->max_width = max_width;
	
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj)
		obj->set_max_width (obj, o->max_width - HTML_CLUEFLOW (o)->indent);

}

static gint
html_clueflow_calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint w;
	gint tempMinWidth=0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		w = obj->calc_min_width (obj);
		if (w > tempMinWidth)
			tempMinWidth = w;
	}
	tempMinWidth += HTML_CLUEFLOW (o)->indent;

	return tempMinWidth;
}

static void
html_clueflow_calc_size (HTMLObject *o, HTMLObject *parent)
{

	HTMLVSpace *vspace;
	HTMLClue *clue = HTML_CLUE (o);
	
	HTMLObject *obj = clue->head;
	HTMLObject *line = clue->head;
	gboolean newLine;
	gint lmargin, rmargin;
	gint oldy;
	gint w, a, d;
	gint runWidth;
	ClearType clear = CNone;
	HTMLObject *run;
	HTMLFitType fit;

	o->ascent = 0;
	o->descent = 0;
	o->width = 0;
	lmargin = HTML_CLUE (parent)->get_left_margin (HTML_CLUE (parent), o->y);
	if (HTML_CLUEFLOW (o)->indent > lmargin)
		lmargin = HTML_CLUEFLOW (o)->indent;
	rmargin = HTML_CLUE (parent)->get_right_margin (HTML_CLUE (parent), o->y);

	w = lmargin;
	a = d = 0;
	newLine = FALSE;

	while (obj != 0) {

		if (obj->flags & NewLine) {
			if (!a)
				a = obj->ascent;
			if (!a && (obj->descent > d))
				d = obj->descent;
			newLine = TRUE;
			vspace = HTML_VSPACE (obj);
			clear = vspace->clear;
			obj = obj->nextObj;
			
		}
		else if (obj->flags & Separator) {
			obj->x = w;

			if (w != lmargin) {
				w += obj->width;
				if (obj->ascent > a)
					a = obj->ascent;
				if (obj->descent > d)
					d = obj->descent;
			}
			obj = obj->nextObj;
		}
		else if (obj->flags & Aligned) {
			HTMLClueAligned *c = (HTMLClueAligned *)obj;

			if (!HTML_CLUE (parent)->appended (HTML_CLUE (parent), c)) {
				obj->calc_size (obj, NULL);
				
				if (HTML_CLUE (HTML_CLUE (c)->halign == Left)) {
					g_print ("Left\n");
				}
				else {
					obj->x = rmargin - obj->width;
					obj->y = o->ascent + obj->ascent;

					HTML_CLUE(parent)->append_right_aligned (parent, c);
				}
			}
			obj = obj->nextObj;
		}
		else {
			runWidth = 0;
			run = obj;
			while ( run && !(run->flags & Separator) && 
				!(run->flags & NewLine) &&
				!(run->flags & Aligned)) {
			  
				run->max_width = rmargin - lmargin;
				fit = run->fit_line (run, (w + runWidth == lmargin),
							    (obj == line),
							    rmargin - runWidth - w);
				if (fit == HTMLNoFit) {
					newLine = TRUE;
					break;
				}
				
				if (run->calc_size)
					run->calc_size (run, o);
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

				run = run->nextObj;

				if (fit == HTMLPartialFit) {
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

				/* Check if the run fits in the current flow area */
				HTML_CLUE (parent)->find_free_area (HTML_CLUE (parent), o->y,line->width, a+d, HTML_CLUEFLOW (o)->indent,
							  &new_y, &new_lmargin, &new_rmargin);
				if ((new_y != o->y) ||
				    (new_lmargin > lmargin) ||
				    (new_rmargin < rmargin)) {
					
					/* We did not get the location we expected 
					   we start building our current line again */
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
					clear = CNone;
				}
				else {
					while (obj != run) {
						obj->x = w;
						w += obj->width;
						obj = obj->nextObj;
					}
				}
			}
			
		}
		
		if ( newLine || !obj) {
			int extra = 0;
			o->ascent += a + d;
			o->y += a + d;
			
			if (w > o->width) {
				o->width = w;
			}
			
			if (clue->halign == HCenter) {
				extra = (rmargin - w) / 2;
				if (extra < 0)
					extra = 0;
			}
			else if (clue->halign == Right) {
				extra = rmargin - w;
				if (extra < 0)
					extra = 0;
			}
			
			while (line != obj) {
				if (!(line->flags & Aligned)) {

					line->y = o->ascent - d;
					if (line->set_max_ascent)
						line->set_max_ascent (line, a);
					if (line->set_max_descent)
						line->set_max_descent (line, d);
					if (clue->halign == HCenter || 
					    clue->halign == Right) {
						line->x += extra;
					}
				}
				line = line->nextObj;
			}

			oldy = o->y;
			
			if (clear == CAll) {
				int new_lmargin, new_rmargin;
				
				HTML_CLUE (parent)->find_free_area (HTML_CLUE (parent), oldy, o->max_width,
							  1, 0, &o->y, &new_lmargin,
							  &new_rmargin);
			}
			else if (clear == CLeft) {
				o->y = html_clue_get_left_clear (parent, oldy);
			}
			else if (clear == CRight) {
				o->y = html_clue_get_right_clear (parent, oldy);
			}

			o->ascent += o->y - oldy;

			lmargin = clue->get_left_margin (clue, o->y);
			if (HTML_CLUEFLOW (o)->indent > lmargin)
				lmargin = HTML_CLUEFLOW (o)->indent;
			rmargin = clue->get_right_margin (clue, o->y);

			w = lmargin;
			d = 0;
			a = 0;
			
			newLine = FALSE;
			clear = CNone;
		}
	}
	
	if (o->width < o->max_width)
		o->width = o->max_width;

}

static gint
html_clueflow_calc_preferred_width (HTMLObject *o) {
	HTMLObject *obj;
	gint maxw = 0, w = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		if (!(obj->flags & NewLine)) {
			w += obj->calc_preferred_width (obj);
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
