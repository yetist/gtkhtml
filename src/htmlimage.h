#ifndef _HTMLIMAGE_H_
#define _HTMLIMAGE_H_

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "htmlobject.h"
#include "htmlengine.h"

typedef struct _HTMLImage HTMLImage;

#define HTML_IMAGE(x) ((HTMLImage *)(x))

struct _HTMLImage {
	HTMLObject parent;

	gboolean predefinedWidth;
	gboolean predefinedHeight;
	gchar *url;
	gint border;
	HTMLEngine *engine;

	GdkPixBuf *pixmap;
};

HTMLObject *html_image_new (HTMLEngine *e, gchar *filename, gint max_width, gint width, gint height,
			    gint percent, gint border);

#endif /* _HTMLIMAGE_H_ */
