#include <gdk/gdk.h>
#include "htmlbullet.h"

void html_bullet_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);

HTMLObject *
html_bullet_new (gint height, gint level, GdkColor color)
{
	HTMLBullet *bullet = g_new0 (HTMLBullet, 1);
	HTMLObject *object = HTML_OBJECT (bullet);
	html_object_init (object, Bullet);

	/* HTMLObject functions */
	object->draw = html_bullet_draw;

	object->ascent = height;
	object->descent = 0;
	object->width = 14;
	bullet->level = level;
	
	return object;
}

void
html_bullet_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	gint xp, yp;

	if (y + height < o->y + o->ascent || y > o->y + o->descent)
		return;

	yp = o->y + ty - 9;
	xp = o->x + tx + 2;

	/* FIXME: should set correct color */

	switch (HTML_BULLET (o)->level) {
	case 1:
		html_painter_draw_ellipse (p, xp, yp, 7, 7);
		break;
	default:
		html_painter_draw_rect (p, xp, yp, 7, 7);
		g_print ("unknown bullet level: %d\n", HTML_BULLET (o)->level);
	}
}


