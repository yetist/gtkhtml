#include "htmlclueh.h"

static void html_clueh_set_max_width (HTMLObject *o, gint w);
static void html_clueh_calc_size (HTMLObject *clue, HTMLObject *parent);
static gint html_clueh_calc_min_width (HTMLObject *o);
static gint html_clueh_calc_preferred_width (HTMLObject *o);

HTMLObject *
html_clueh_new (gint x, gint y, gint max_width)
{
	HTMLClueH *clueh = g_new0 (HTMLClueH, 1);
	HTMLObject *object = HTML_OBJECT (clueh);
	HTMLClue *clue = HTML_CLUE (clueh);
	html_clue_init (clue, ClueH);

	/* HTMLObject functions */
	object->set_max_width = html_clueh_set_max_width;
	object->calc_size = html_clueh_calc_size;
	object->calc_min_width = html_clueh_calc_min_width;
	object->calc_preferred_width = html_clueh_calc_preferred_width;

	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->percent = 100;

	return object;
}

static void
html_clueh_set_max_width (HTMLObject *o, gint w)
{
	HTMLObject *obj;
	o->max_width = w;

	/* First calculate width minus fixed width objects */
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		if (obj->percent <= 0) /* i.e. fixed width objects */
			w -= obj->width;
	}

	/* Now call set_max_width for variable objects */
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		if (obj->percent > 0)
			obj->set_max_width (obj, w - HTML_CLUEH (o)->indent);
	}
}

static void
html_clueh_calc_size (HTMLObject *clue, HTMLObject *parent)
{
	HTMLObject *obj;

	gint lmargin = 0;
	gint a = 0, d = 0;

	clue->set_max_width (clue, clue->max_width);

	html_clue_calc_size (clue, parent);

	/* FIXME : Why should this be needed?
	   parent->calc_size (parent, parent); */

	if (parent)
		lmargin = HTML_CLUE (parent)->get_left_margin (HTML_CLUE (parent), clue->y);
	clue->width = lmargin + HTML_CLUEH (clue)->indent;
	clue->descent = 0;
	clue->ascent = 0;

	for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->nextObj) {
		obj->fit_line (obj, (obj == HTML_CLUE (clue)->head), TRUE, -1);
		obj->x = clue->width;
		clue->width += obj->width;
		if (obj->ascent > a)
			a = obj->ascent;
		if (obj->descent > d)
			d = obj->descent;
	}

	clue->ascent = a + d;

	switch (HTML_CLUE (clue)->valign) {
	case Top:
		for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->nextObj)
			obj->y = obj->ascent;
		break;

	case VCenter:
		for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->nextObj)
			obj->y = clue->ascent / 2;
		break;

	default:
		for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->nextObj)
			obj->y = clue->ascent - d;
	}
}

static gint
html_clueh_calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint minWidth = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj)
		minWidth += obj->calc_min_width (obj);

	return minWidth + HTML_CLUEH (o)->indent;
}

static gint
html_clueh_calc_preferred_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint prefWidth = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) 
		prefWidth += obj->calc_preferred_width (obj);

	return prefWidth + HTML_CLUEH (o)->indent;
}
