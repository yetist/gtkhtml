#include <string.h>
#include "htmlobject.h"
#include "htmlfont.h"
#include "htmlclue.h"
#include "htmltext.h"
#include "htmltextmaster.h"
#include "htmlclueflow.h"
#include "htmlcluev.h"
#include "htmlpainter.h"
#include "htmlrule.h"
#include "htmlclue.h"
#include "debug.h"

static void html_object_destroy (HTMLObject *o);
static gint html_object_calc_min_width (HTMLObject *o);
static void html_object_set_max_width(HTMLObject *o, gint max_width);
static HTMLFitType html_object_fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun, gint widthLeft);
static void html_object_reset (HTMLObject *o);
static gint html_object_calc_preferred_width (HTMLObject *o);
static void html_object_calc_absolute_pos (HTMLObject *o, gint x, gint y);

static void
html_object_destroy (HTMLObject *o)
{
	g_free (o);
}

void
html_object_init (HTMLObject *o, objectType ObjectType)
{
	o->ObjectType = ObjectType;

	/* Default HTMLObject functions */
	o->fit_line = html_object_fit_line;
	o->destroy = html_object_destroy;
	o->set_max_width = html_object_set_max_width;
	o->reset = html_object_reset;
	o->calc_min_width = html_object_calc_min_width;
	o->calc_preferred_width = html_object_calc_preferred_width;
	o->calc_absolute_pos = html_object_calc_absolute_pos;

	/* Default variables */
	o->flags = 0;
	o->flags |= FixedWidth;
	o->max_width = 0;
	o->width = 0;
	o->ascent = 0;
	o->descent = 0;
	o->objCount++;
	o->nextObj = 0;
	o->x = 0;
	o->y = 0;
}

HTMLObject *
html_object_new (void)
{
	HTMLObject *o;
	
	o = g_new0 (HTMLObject, 1);

	html_object_init (o, Object);

	return o;
}

static void
html_object_calc_absolute_pos (HTMLObject *o, gint x, gint y)
{
	o->absX = x + o->x;
	o->absY = y + o->y - o->ascent;
}

static void
html_object_set_max_width(HTMLObject *o, gint max_width)
{
	o->max_width = max_width;
}

static HTMLFitType
html_object_fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun, gint widthLeft)
{
	return HTMLCompleteFit;
}

static void
html_object_reset (HTMLObject *o)
{
	/* FIXME: Do something here */
}

static gint
html_object_calc_min_width (HTMLObject *o)
{
	return o->width;
}

static gint
html_object_calc_preferred_width (HTMLObject *o)
{
	return o->width;
}
