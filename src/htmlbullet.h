#ifndef _HTMLBULLET_H_
#define _HTMLBULLET_H_

#include <gdk/gdk.h>
#include "htmlobject.h"

typedef struct _HTMLBullet HTMLBullet;

#define HTML_BULLET(x) ((HTMLBullet *)(x))

struct _HTMLBullet {
	HTMLObject parent;
	
	gchar level;
	GdkColor *color;
};

HTMLObject *html_bullet_new (gint height, gint level, GdkColor color);

#endif /* _HTMLBULLET_H_ */
