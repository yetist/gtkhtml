/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries
    Copyright (C) 1999 Anders Carlsson (andersca@gnu.org)
              (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#include <gdk/gdk.h>
#include "htmlsettings.h"

HTMLSettings *
html_settings_new (void)
{
	HTMLSettings *s = g_new0 (HTMLSettings, 1);
	
	s->fontBaseSize = 3;

	s->fontbasecolor.red = 0;
	s->fontbasecolor.green = 0;
	s->fontbasecolor.blue = 0;

	s->bgcolor.red = 0xff;
	s->bgcolor.green = 0xff;
	s->bgcolor.blue = 0xff;

	return s;
}

void
html_settings_destroy (HTMLSettings *settings)
{
	g_return_if_fail (settings != NULL);

	g_free (settings);
}

void
html_settings_set_bgcolor (HTMLSettings *settings, GdkColor *color)
{
	g_return_if_fail (settings != NULL);
	g_return_if_fail (color != NULL);

	settings->bgcolor = *color;
}
