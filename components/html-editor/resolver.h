/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef RESOLVER_H_
#define RESOLVER_H_

#include <libgnome/gnome-defs.h>
#include <bonobo/bonobo-object.h>

BEGIN_GNOME_DECLS

#define HTMLEDITOR_RESOLVER_TYPE        (htmleditor_resolver_get_type ())
#define HTMLEDITOR_RESOLVER(o)          (GTK_CHECK_CAST ((o), HTMLEDITOR_RESOLVER_TYPE, HTMLEditorResolver))
#define HTMLEDITOR_RESOLVER_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), HTMLEDITOR_RESOLVER_TYPE, HTMLResolverClass))
#define IS_HTMLEDITOR_RESOLVER(o)       (GTK_CHECK_TYPE ((o), HTMLEDITOR_RESOLVER_TYPE))
#define IS_HTMLEDITOR_RESOLVER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), HTMLEDITOR_RESOLVER_TYPE))

typedef struct {
	BonoboObject parent;

	char *instance_data;
} HTMLEditorResolver;

typedef struct {
	BonoboObjectClass parent_class;
} HTMLEditorResolverClass;

GtkType                     htmleditor_resolver_get_type  (void);
HTMLEditorResolver         *htmleditor_resolver_construct (HTMLEditorResolver *resolver, HTMLEditor_Resolver corba_resolver);
HTMLEditorResolver         *htmleditor_resolver_new       (void);

POA_HTMLEditor_Resolver__epv *htmleditor_resolver_get_epv (void);

END_GNOME_DECLS

#endif /* RESOLVER_H_ */








