/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of gnome-spell bonobo component

    Copyright (C) 2000 Helix Code, Inc.
    Authors:           Radek Doulik <rodo@helixcode.com>

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

#ifndef ENGINE_H_
#define ENGINE_H_

#include <libgnome/gnome-defs.h>
#include <bonobo/bonobo-object.h>
#include "HTMLEditor.h"
#include "gtkhtml.h"

BEGIN_GNOME_DECLS

#define HTMLEDITOR_ENGINE_TYPE        (htmleditor_engine_get_type ())
#define HTMLEDITOR_ENGINE(o)          (GTK_CHECK_CAST ((o), HTMLEDITOR_ENGINE_TYPE, HTMLEditorEngine))
#define HTMLEDITOR_ENGINE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), HTMLEDITOR_ENGINE_TYPE, HTMLEditorEngineClass))
#define IS_HTMLEDITOR_ENGINE(o)       (GTK_CHECK_TYPE ((o), HTMLEDITOR_ENGINE_TYPE))
#define IS_HTMLEDITOR_ENGINE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), HTMLEDITOR_ENGINE_TYPE))

typedef struct {
	BonoboObject parent;

	GtkHTML *html;

	BonoboObjectClient *listener_client;
	GNOME_HTMLEditor_Listener listener;
} HTMLEditorEngine;

typedef struct {
	BonoboObjectClass parent_class;
} HTMLEditorEngineClass;

GtkType                           htmleditor_engine_get_type   (void);
HTMLEditorEngine                 *htmleditor_engine_construct  (HTMLEditorEngine        *engine,
								GNOME_HTMLEditor_Engine  corba_engine);
HTMLEditorEngine                 *htmleditor_engine_new        (GtkHTML                 *html);
POA_GNOME_HTMLEditor_Engine__epv *htmleditor_engine_get_epv    (void);

END_GNOME_DECLS

#endif /* ENGINE_H_ */
