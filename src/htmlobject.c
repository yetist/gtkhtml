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
