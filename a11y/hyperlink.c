/*  This file is part of the GtkHTML library.
 *
 *  Copyright 2002 Ximian, Inc.
 *
 *  Author: Radek Doulik
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <atk/atkhyperlink.h>

#include "htmltext.h"

#include "object.h"
#include "html.h"
#include "hyperlink.h"

struct _HTMLA11YHyperLink {
	AtkHyperlink atk_hyper_link;

	/* use the union for valid type-punning */
	union {
		HTMLA11Y *object;
		gpointer weakref;
	} a11y;
	gint num;
	gint offset;
	gchar *description;
};


static void atk_action_interface_init (AtkActionIface *iface);

static gboolean html_a11y_hyper_link_do_action (AtkAction *action, gint i);
static gint html_a11y_hyper_link_get_n_actions (AtkAction *action);
static const gchar * html_a11y_hyper_link_get_description (AtkAction *action, gint i);
static const gchar * html_a11y_hyper_link_get_name (AtkAction *action, gint i);
static gboolean html_a11y_hyper_link_set_description (AtkAction *action, gint i, const gchar *description);

G_DEFINE_TYPE_EXTENDED (HTMLA11YHyperLink,
                       html_a11y_hyper_link,
                       ATK_TYPE_HYPERLINK,
                       0,
                       G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION,
                         atk_action_interface_init));

static void
atk_action_interface_init (AtkActionIface *iface)
{
	g_return_if_fail (iface != NULL);

	iface->do_action       = html_a11y_hyper_link_do_action;
	iface->get_n_actions   = html_a11y_hyper_link_get_n_actions;
	iface->get_description = html_a11y_hyper_link_get_description;
	iface->get_name        = html_a11y_hyper_link_get_name;
	iface->set_description = html_a11y_hyper_link_set_description;
}

static void
html_a11y_hyper_link_finalize (GObject *obj)
{
	HTMLA11YHyperLink *hl = HTML_A11Y_HYPER_LINK (obj);

	if (hl->a11y.object)
		g_object_remove_weak_pointer (G_OBJECT (hl->a11y.object),
					      &hl->a11y.weakref);

	G_OBJECT_CLASS (html_a11y_hyper_link_parent_class)->finalize (obj);
}

static gint
html_a11y_hyper_link_get_start_index (AtkHyperlink *link)
{
	HTMLA11YHyperLink *hl = HTML_A11Y_HYPER_LINK (link);
	HTMLText *text = HTML_TEXT (HTML_A11Y_HTML (hl->a11y.object));
	Link *a = (Link *) g_slist_nth_data (text->links, hl->num);
	return a ? a->start_offset : -1;
}

static gint
html_a11y_hyper_link_get_end_index (AtkHyperlink *link)
{
	HTMLA11YHyperLink *hl = HTML_A11Y_HYPER_LINK (link);
	Link *a = (Link *) g_slist_nth_data (HTML_TEXT (HTML_A11Y_HTML (hl->a11y.object))->links, hl->num);
	return a ? a->end_offset : -1;
}

static void
html_a11y_hyper_link_class_init (HTMLA11YHyperLinkClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkHyperlinkClass *atk_hyperlink_class = ATK_HYPERLINK_CLASS (klass);
	html_a11y_hyper_link_parent_class = g_type_class_peek_parent (klass);

	atk_hyperlink_class->get_start_index = html_a11y_hyper_link_get_start_index;
	atk_hyperlink_class->get_end_index = html_a11y_hyper_link_get_end_index;
	gobject_class->finalize = html_a11y_hyper_link_finalize;
}

static void
html_a11y_hyper_link_init (HTMLA11YHyperLink *a11y_hyper_link)
{
	a11y_hyper_link->description = NULL;
}

AtkHyperlink *
html_a11y_hyper_link_new (HTMLA11Y *a11y,
                          gint link_index)
{
	HTMLA11YHyperLink *hl;

	g_return_val_if_fail (G_IS_HTML_A11Y (a11y), NULL);

	hl = HTML_A11Y_HYPER_LINK (g_object_new (G_TYPE_HTML_A11Y_HYPER_LINK, NULL));

	hl->a11y.object = a11y;
	hl->num = link_index;
	hl->offset = ((Link *) g_slist_nth_data (HTML_TEXT (HTML_A11Y_HTML (a11y))->links, link_index))->start_offset;
	g_object_add_weak_pointer (G_OBJECT (hl->a11y.object), &hl->a11y.weakref);

	return ATK_HYPERLINK (hl);
}

/*
 * Action interface
 */

static gboolean
html_a11y_hyper_link_do_action (AtkAction *action,
                                gint i)
{
	HTMLA11YHyperLink *hl;
	gboolean result = FALSE;

	hl = HTML_A11Y_HYPER_LINK (action);

	if (i == 0 && hl->a11y.object) {
		HTMLText *text = HTML_TEXT (HTML_A11Y_HTML (hl->a11y.object));
		gchar *url = html_object_get_complete_url (HTML_OBJECT (text), hl->offset);

		if (url && *url) {
			GObject *gtkhtml = GTK_HTML_A11Y_GTKHTML_POINTER
				(html_a11y_get_gtkhtml_parent (HTML_A11Y (hl->a11y.object)));

			g_signal_emit_by_name (gtkhtml, "link_clicked", url);
			result = TRUE;
		}

		g_free (url);
	}

	return result;
}

static gint
html_a11y_hyper_link_get_n_actions (AtkAction *action)
{
	return 1;
}

static const gchar *
html_a11y_hyper_link_get_description (AtkAction *action,
                                      gint i)
{
	if (i == 0) {
		HTMLA11YHyperLink *hl;

		hl = HTML_A11Y_HYPER_LINK (action);

		return hl->description;
	}

	return NULL;
}

static const gchar *
html_a11y_hyper_link_get_name (AtkAction *action,
                               gint i)
{
	return i == 0 ? "link click" : NULL;
}

static gboolean
html_a11y_hyper_link_set_description (AtkAction *action,
                                      gint i,
                                      const gchar *description)
{
	if (i == 0) {
		HTMLA11YHyperLink *hl;

		hl = HTML_A11Y_HYPER_LINK (action);

		g_free (hl->description);
		hl->description = g_strdup (description);

		return TRUE;
	}

	return FALSE;
}
