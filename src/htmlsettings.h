#ifndef _HTMLSETTINGS_H_
#define _HTMLSETTINGS_H_

#include <gdk/gdk.h>

typedef struct _HTMLSettings HTMLSettings;

struct _HTMLSettings {
	gint fontBaseSize;
	GdkColor *bgcolor;
};

HTMLSettings *html_settings_new (void);

#endif /* _HTMLSETTINGS_H_ */
