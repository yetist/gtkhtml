#include "htmlvspace.h"

HTMLObject *
html_vspace_new (gint space, ClearType clear)
{
	HTMLVSpace *vspace = g_new0 (HTMLVSpace, 1);
	HTMLObject *object = HTML_OBJECT (vspace);
	html_object_init (object, VSpace);
	
	object->ascent = space;
	object->descent = 0;
	object->width = 1;
	object->flags |= NewLine;
	
	vspace->clear = clear;

	return object;
}
