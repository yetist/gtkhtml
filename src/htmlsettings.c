#include <gdk/gdk.h>
#include "htmlsettings.h"

HTMLSettings 
*html_settings_new (void)
{
	HTMLSettings *s = g_new0 (HTMLSettings, 1);
	
	s->fontBaseSize = 3;
	return s;
}
