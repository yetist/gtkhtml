/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.

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

#include "htmlengine-edit-styles.h"


HTMLClueFlowStyle
html_engine_get_current_clueflow_style (HTMLEngine *engine)
{
	HTMLObject *current;
	HTMLObject *parent;

	g_return_val_if_fail (engine != NULL, HTML_CLUEFLOW_STYLE_NORMAL);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), HTML_CLUEFLOW_STYLE_NORMAL);

	current = engine->cursor->object;
	if (current == NULL)
		return HTML_CLUEFLOW_STYLE_NORMAL;

	parent = current->parent;
	if (parent == NULL)
		return HTML_CLUEFLOW_STYLE_NORMAL;

	if (HTML_OBJECT_TYPE (parent) != HTML_TYPE_CLUEFLOW)
		return HTML_CLUEFLOW_STYLE_NORMAL;

	return HTML_CLUEFLOW (parent)->style;
}

gboolean
html_engine_set_clueflow_style (HTMLEngine *engine,
				HTMLClueFlowStyle style)
{
	HTMLObject *current;
	HTMLObject *parent;

	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), FALSE);

	current = engine->cursor->object;
	if (current == NULL)
		return FALSE;

	parent = current->parent;
	if (parent == NULL)
		return FALSE;

	if (HTML_OBJECT_TYPE (parent) != HTML_TYPE_CLUEFLOW)
		return FALSE;

	html_clueflow_set_style (HTML_CLUEFLOW (parent), engine, style);

	return TRUE;
}
