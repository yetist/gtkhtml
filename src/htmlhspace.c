#include "htmlobject.h"
#include "htmlhspace.h"

void
html_hspace_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLHSpace *hspace = HTML_HSPACE (o);

	html_painter_set_font (p, hspace->font);

	/* FIXME: Sane Check */
	
	html_painter_draw_text (p, o->x + tx, o->y + ty, " ", 1);
}

HTMLObject *
html_hspace_new (HTMLFont *font, HTMLPainter *painter, gboolean hidden)
{
	HTMLHSpace *hspace = g_new0 (HTMLHSpace, 1);
	HTMLObject *object = HTML_OBJECT (hspace);
	html_object_init (object, HSpace);
	
	/* HTMLObject functions */
	object->draw = html_hspace_draw;

	object->ObjectType = HSpace;
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font) + 1;

	hspace->font = font;

	if (!hidden)
		object->width = html_font_calc_width (font, " ", -1);
	else
		object->width = 0;

	object->flags |= Separator;
	object->flags &= ~Hidden;

	return object;
}
