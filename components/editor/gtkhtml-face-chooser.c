/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-face-chooser.c
 *
 * Copyright (C) 2008 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gtkhtml-face-chooser.h"

#include <glib/gi18n-lib.h>

/* Constant version of GtkhtmlFace. */
typedef struct {
	const gchar *label;
	const gchar *icon_name;
	const gchar *text_face;
} ConstantFace;

static ConstantFace available_faces[] = {
	/* Translators: :-) */
	{ N_("_Smile"),		"face-smile",		":-)"	},
	/* Translators: :-( */
	{ N_("S_ad"),		"face-sad",		":-("	},
	/* Translators: ;-) */
	{ N_("_Wink"),		"face-wink",		";-)"	},
	/* Translators: :-P */
	{ N_("Ton_gue"),	"face-raspberry",	":-P"	},
	/* Translators: :-)) */
	{ N_("Laug_h"),		"face-laugh",		":-))"	},
	/* Translators: :-| */
	{ N_("_Plain"),		"face-plain",		":-|"	},
	/* Translators: :-! */
	{ N_("Smi_rk"),		"face-smirk",		":-!"	},
	/* Translators: :"-) */
	{ N_("_Embarrassed"),	"face-embarrassed",	":\"-)"	},
	/* Translators: :-D */
	{ N_("_Big Smile"),	"face-smile-big",	":-D"	},
	/* Translators: :-/ */
	{ N_("Uncer_tain"),	"face-uncertain",	":-/"	},
	/* Translators: :-O */
	{ N_("S_urprise"),	"face-surprise",	":-O"	},
	/* Translators: :-S */
	{ N_("W_orried"),	"face-worried",		":-S"	},
	/* Translators: :-* */
	{ N_("_Kiss"),		"face-kiss",		":-*"	},
	/* Translators: X-( */
	{ N_("A_ngry"),		"face-angry",		"X-("	},
	/* Translators: B-) */
	{ N_("_Cool"),		"face-cool",		"B-)"	},
	/* Translators: O:-) */
	{ N_("Ange_l"),		"face-angel",		"O:-)"	},
	/* Translators: :'( */
	{ N_("Cr_ying"),	"face-crying",		":'("	},
	/* Translators: :-Q */
	{ N_("S_ick"),		"face-sick",		":-Q"	},
	/* Translators: |-) */
	{ N_("Tire_d"),		"face-tired",		"|-)"	},
	/* Translators: >:-) */
	{ N_("De_vilish"),	"face-devilish",	">:-)"	},
	/* Translators: :-(|) */
	{ N_("_Monkey"),	"face-monkey",		":-(|)"	}
};

enum {
	ITEM_ACTIVATED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
face_chooser_class_init (GtkhtmlFaceChooserIface *iface)
{
	g_object_interface_install_property (
		iface,
		g_param_spec_boxed (
			"current-face",
			"Current Face",
			"Currently selected face",
			GTKHTML_TYPE_FACE,
			G_PARAM_READWRITE));

	signals[ITEM_ACTIVATED] = g_signal_new (
		"item-activated",
		G_TYPE_FROM_INTERFACE (iface),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkhtmlFaceChooserIface, item_activated),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

GType
gtkhtml_face_chooser_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		static const GTypeInfo type_info = {
			sizeof (GtkhtmlFaceChooserIface),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) face_chooser_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,  /* class_data */
			0,     /* instance_size */
			0,     /* n_preallocs */
			NULL,  /* instance_init */
			NULL   /* value_table */
		};

		type = g_type_register_static (
			G_TYPE_INTERFACE, "GtkhtmlFaceChooser", &type_info, 0);

		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

GtkhtmlFace *
gtkhtml_face_chooser_get_current_face (GtkhtmlFaceChooser *chooser)
{
	GtkhtmlFaceChooserIface *iface;

	g_return_val_if_fail (GTKHTML_IS_FACE_CHOOSER (chooser), NULL);

	iface = GTKHTML_FACE_CHOOSER_GET_IFACE (chooser);
	g_return_val_if_fail (iface->get_current_face != NULL, NULL);

	return iface->get_current_face (chooser);
}

void
gtkhtml_face_chooser_set_current_face (GtkhtmlFaceChooser *chooser,
                                       GtkhtmlFace *face)
{
	GtkhtmlFaceChooserIface *iface;

	g_return_if_fail (GTKHTML_IS_FACE_CHOOSER (chooser));

	iface = GTKHTML_FACE_CHOOSER_GET_IFACE (chooser);
	g_return_if_fail (iface->set_current_face != NULL);

	iface->set_current_face (chooser, face);
}

void
gtkhtml_face_chooser_item_activated (GtkhtmlFaceChooser *chooser)
{
	g_return_if_fail (GTKHTML_IS_FACE_CHOOSER (chooser));

	g_signal_emit (chooser, signals[ITEM_ACTIVATED], 0);
}

GList *
gtkhtml_face_chooser_get_items (GtkhtmlFaceChooser *chooser)
{
	GList *list = NULL;
	gint ii;

	for (ii = 0; ii < G_N_ELEMENTS (available_faces); ii++)
		list = g_list_prepend (list, &available_faces[ii]);

	return g_list_reverse (list);
}
