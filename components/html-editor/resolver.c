/* -*- Mode: IDL; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *
 * Author:
 *   Larry Ewing (lewing@helixcode.com)
 *
 * This file is here to show what are the basic steps into create a Bonobo Component.
 */
#include <config.h>
#include <bonobo.h>

/*
 * This pulls the CORBA definitions for the Demo::Echo server
 */
#include "HTMLEditor.h"

/*
 * This pulls the definition for the BonoboObject (Gtk Type)
 */
#include "resolver.h"

static BonoboObjectClass *resolver_parent_class;
static POA_HTMLEditor_Resolver__vepv htmleditor_resolver_vepv;

static void
impl_HTMLEditor_Resolver_loadURL (PortableServer_Servant servant,
				  const Bonobo_Stream stream,
				  const CORBA_char *url,
				  CORBA_Environment *ev)
{
	HTMLEditorResolver * resolver= HTMLEDITOR_RESOLVER (bonobo_object_from_servant (servant));
	
	if (!strncmp (url, "/dev/null", 6)) {
		g_warning ("how about a url: %s", url);
	} else {
		g_warning ("perhaps this sounds better: %s", url);
		CORBA_exception_set (ev,
				     CORBA_USER_EXCEPTION,
				     ex_HTMLEditor_Resolver_NotFound,
				     NULL);
	}
	
		
}

POA_HTMLEditor_Resolver__epv *htmleditor_resolver_get_epv (void)
{
	POA_HTMLEditor_Resolver__epv *epv;

	epv = g_new0 (POA_HTMLEditor_Resolver__epv, 1);
		
	epv->loadURL = impl_HTMLEditor_Resolver_loadURL;

	return epv;
}


static void
init_htmleditor_resolver_corba_class (void)
{
	htmleditor_resolver_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	htmleditor_resolver_vepv.HTMLEditor_Resolver_epv = htmleditor_resolver_get_epv ();
}

static void
htmleditor_resolver_class_init (HTMLEditorResolverClass *resolver_class)
{
	resolver_parent_class = gtk_type_class (bonobo_object_get_type ());
	init_htmleditor_resolver_corba_class ();
}

HTMLEditor_Resolver
htmleditor_resolver_corba_object_create (BonoboObject *object)
{
	POA_HTMLEditor_Resolver *servant;
	CORBA_Environment ev;

	servant = (POA_HTMLEditor_Resolver *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &htmleditor_resolver_vepv;

	CORBA_exception_init (&ev);
	POA_HTMLEditor_Resolver__init ((PortableServer_Servant) servant, &ev);
	ORBIT_OBJECT_KEY(servant->_private)->object = NULL;

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);

	return (HTMLEditor_Resolver) bonobo_object_activate_servant (object, servant);
}

GtkType
htmleditor_resolver_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"HTMLEditorResolver",
			sizeof (HTMLEditorResolver),
			sizeof (HTMLEditorResolverClass),
			(GtkClassInitFunc) htmleditor_resolver_class_init,
			(GtkObjectInitFunc) NULL,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};
		
		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}
	return type;
}

HTMLEditorResolver *
htmleditor_resolver_new (void)
{
	HTMLEditorResolver *resolver;
	HTMLEditor_Resolver *corba_resolver;

	resolver = gtk_type_new (htmleditor_resolver_get_type ());

	corba_resolver = htmleditor_resolver_corba_object_create (BONOBO_OBJECT (resolver));

	if (corba_resolver == CORBA_OBJECT_NIL) {
		gtk_object_destroy (GTK_OBJECT (resolver));
		return NULL;
	}

	return resolver;
}




