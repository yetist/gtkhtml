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

#include <config.h>
#include <bonobo.h>

#include "htmlengine-edit.h"
#include "htmltext.h"
#include "engine.h"

static BonoboObjectClass *engine_parent_class;
static POA_GNOME_HTMLEditor_Engine__vepv engine_vepv;

inline static HTMLEditorEngine *
html_editor_engine_from_servant (PortableServer_Servant servant)
{
	return HTMLEDITOR_ENGINE (bonobo_object_from_servant (servant));
}

static CORBA_any *
impl_get_paragraph_data (PortableServer_Servant servant, const CORBA_char * key, CORBA_Environment * ev)
{
	HTMLEditorEngine *e = html_editor_engine_from_servant (servant);
	CORBA_boolean value;
	CORBA_any *rv = NULL;

	value = (CORBA_boolean) GPOINTER_TO_INT
		(e->html->engine->cursor->object && e->html->engine->cursor->object->parent
		&& HTML_OBJECT_TYPE (e->html->engine->cursor->object->parent) == HTML_TYPE_CLUEFLOW
		? html_object_get_data (e->html->engine->cursor->object->parent, key) : NULL);

	rv = bonobo_arg_new (TC_boolean);
	BONOBO_ARG_SET_BOOLEAN (rv, value);

	/* printf ("get paragraph data\n"); */

	return rv;
}

static void
impl_set_paragraph_data (PortableServer_Servant servant,
			 const CORBA_char * key, const CORBA_any * value,
			 CORBA_Environment * ev)
{
	HTMLEditorEngine *e = html_editor_engine_from_servant (servant);

	if (e->html->engine->cursor->object && e->html->engine->cursor->object->parent
	    && HTML_OBJECT_TYPE (e->html->engine->cursor->object->parent) == HTML_TYPE_CLUEFLOW) {
		html_object_set_data (HTML_OBJECT (e->html->engine->cursor->object->parent), key,
				      GINT_TO_POINTER ((gboolean) BONOBO_ARG_GET_BOOLEAN (value)));
	}
}

static void
impl_set_object_data_by_type (PortableServer_Servant servant,
			 const CORBA_char * type_name, const CORBA_char * key, const CORBA_any * data,
			 CORBA_Environment * ev)
{
	HTMLEditorEngine *e = html_editor_engine_from_servant (servant);

	/* printf ("set data by type\n"); */

	/* FIXME should use bonobo_arg_to_gtk, but this seems to be broken */
	html_engine_set_data_by_type (e->html->engine, html_type_from_name (type_name), key,
				      GINT_TO_POINTER ((gint) BONOBO_ARG_GET_BOOLEAN (data)));
}

static void
unref_listener (HTMLEditorEngine *e)
{
	if (e->listener_client != CORBA_OBJECT_NIL)
		bonobo_object_unref (BONOBO_OBJECT (e->listener_client));
}

static void
impl_set_listener (PortableServer_Servant servant, const GNOME_HTMLEditor_Listener value, CORBA_Environment * ev)
{
	HTMLEditorEngine *e = html_editor_engine_from_servant (servant);

	/* printf ("set listener\n"); */

	unref_listener (e);
	e->listener_client = bonobo_object_client_from_corba (value);
	e->listener        = bonobo_object_client_query_interface (e->listener_client, "IDL:GNOME/HTMLEditor/Listener:1.0", ev);
}

static GNOME_HTMLEditor_Listener
impl_get_listener (PortableServer_Servant servant, CORBA_Environment * ev)
{
	return html_editor_engine_from_servant (servant)->listener;
}

static void
impl_run_command (PortableServer_Servant servant, const CORBA_char * command, CORBA_Environment * ev)
{
	HTMLEditorEngine *e = html_editor_engine_from_servant (servant);

	/* printf ("command: %s\n", command); */

	gtk_html_editor_command (e->html, command);
}

static CORBA_boolean
par_is_empty (HTMLClue *clue)
{
	if (clue->head && clue->head->next == clue->tail
	    && html_object_is_text (clue->head) && !*HTML_TEXT (clue->head)->text
	    && HTML_OBJECT_TYPE (clue->tail) == HTML_TYPE_TEXTSLAVE)
		return CORBA_TRUE;
	return CORBA_FALSE;
}

static CORBA_boolean
impl_is_paragraph_empty (PortableServer_Servant servant, CORBA_Environment * ev)
{
	HTMLEditorEngine *e = html_editor_engine_from_servant (servant);

	if (e->html->engine->cursor->object
	    && e->html->engine->cursor->object->parent
	    && HTML_OBJECT_TYPE (e->html->engine->cursor->object->parent) == HTML_TYPE_CLUEFLOW) {
		return par_is_empty (HTML_CLUE (e->html->engine->cursor->object->parent));
	}
	return CORBA_FALSE;
}

static CORBA_boolean
impl_is_previous_paragraph_empty (PortableServer_Servant servant, CORBA_Environment * ev)
{
	HTMLEditorEngine *e = html_editor_engine_from_servant (servant);

	if (e->html->engine->cursor->object
	    && e->html->engine->cursor->object->parent
	    && e->html->engine->cursor->object->parent->prev
	    && HTML_OBJECT_TYPE (e->html->engine->cursor->object->parent->prev) == HTML_TYPE_CLUEFLOW) {
		return par_is_empty (HTML_CLUE (e->html->engine->cursor->object->parent->prev));
	}
	return CORBA_FALSE;
}


POA_GNOME_HTMLEditor_Engine__epv *
htmleditor_engine_get_epv (void)
{
	POA_GNOME_HTMLEditor_Engine__epv *epv;

	epv = g_new0 (POA_GNOME_HTMLEditor_Engine__epv, 1);

	epv->_set_listener            = impl_set_listener;
	epv->_get_listener            = impl_get_listener;
	epv->setParagraphData         = impl_set_paragraph_data;
	epv->getParagraphData         = impl_get_paragraph_data;
	epv->setObjectDataByType      = impl_set_object_data_by_type;
	epv->runCommand               = impl_run_command;
	epv->isParagraphEmpty         = impl_is_paragraph_empty;
	epv->isPreviousParagraphEmpty = impl_is_previous_paragraph_empty;

	return epv;
}

static void
init_engine_corba_class (void)
{
	engine_vepv.Bonobo_Unknown_epv    = bonobo_object_get_epv ();
	engine_vepv.GNOME_HTMLEditor_Engine_epv = htmleditor_engine_get_epv ();
}

static void
engine_object_init (GtkObject *object)
{
	HTMLEditorEngine *e = HTMLEDITOR_ENGINE (object);

	e->listener_client = CORBA_OBJECT_NIL;
}

static void
engine_object_destroy (GtkObject *object)
{
	HTMLEditorEngine *e = HTMLEDITOR_ENGINE (object);

	unref_listener (e);

	GTK_OBJECT_CLASS (engine_parent_class)->destroy (object);
}

static void
engine_class_init (HTMLEditorEngineClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	engine_parent_class   = gtk_type_class (bonobo_object_get_type ());
	object_class->destroy = engine_object_destroy;

	init_engine_corba_class ();
}

GtkType
htmleditor_engine_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"HTMLEditorEngine",
			sizeof (HTMLEditorEngine),
			sizeof (HTMLEditorEngineClass),
			(GtkClassInitFunc) engine_class_init,
			(GtkObjectInitFunc) engine_object_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

HTMLEditorEngine *
htmleditor_engine_construct (HTMLEditorEngine *engine, GNOME_HTMLEditor_Engine corba_engine)
{
	g_return_val_if_fail (engine != NULL, NULL);
	g_return_val_if_fail (IS_HTMLEDITOR_ENGINE (engine), NULL);
	g_return_val_if_fail (corba_engine != CORBA_OBJECT_NIL, NULL);

	engine->listener_client = CORBA_OBJECT_NIL;

	if (!bonobo_object_construct (BONOBO_OBJECT (engine), (CORBA_Object) corba_engine))
		return NULL;

	return engine;
}

static GNOME_HTMLEditor_Engine
create_engine (BonoboObject *engine)
{
	POA_GNOME_HTMLEditor_Engine *servant;
	CORBA_Environment ev;

	servant = (POA_GNOME_HTMLEditor_Engine *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &engine_vepv;

	CORBA_exception_init (&ev);
	POA_GNOME_HTMLEditor_Engine__init ((PortableServer_Servant) servant, &ev);
	ORBIT_OBJECT_KEY(servant->_private)->object = NULL;

	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);

	return (GNOME_HTMLEditor_Engine) bonobo_object_activate_servant (engine, servant);
}

HTMLEditorEngine *
htmleditor_engine_new (GtkHTML *html)
{
	HTMLEditorEngine *engine;
	GNOME_HTMLEditor_Engine corba_engine;

	engine = gtk_type_new (HTMLEDITOR_ENGINE_TYPE);
	engine->html = html;

	corba_engine = create_engine (BONOBO_OBJECT (engine));

	if (corba_engine == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (engine));
		return NULL;
	}
	
	return htmleditor_engine_construct (engine, corba_engine);
}
