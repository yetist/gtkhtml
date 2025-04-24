/*  This file is part of the GtkHTML library.
 *
 *  Copyright 2002 Ximian, Inc.
 *  Copyright 2025 Xiaotian Wu <yetist@gmail.com>
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

#include "gtkhtml.h"
#include "htmlengine.h"
#include "htmlobject.h"

#include "object.h"
#include "paragraph.h"
#include "utils.h"
#include "text.h"
#include <glib/gi18n-lib.h>

struct _GtkHTMLA11Y
{
	GtkContainerAccessible parent;
};

static void gtk_html_a11y_initialize (AtkObject *obj,
                                      gpointer data);
static void atk_action_interface_init (AtkActionIface *iface);

G_DEFINE_TYPE_EXTENDED (GtkHTMLA11Y,
                       gtk_html_a11y,
                       GTK_TYPE_CONTAINER_ACCESSIBLE,
                       0,
                       G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION,
                         atk_action_interface_init));

static gint
get_n_actions (AtkAction *action)
{
	return 1;
}

static const gchar *
get_description (AtkAction *action,
                 gint i)
{
	if (i == 0)
		return _("grab focus");

	return NULL;
}

static const gchar *
action_get_name (AtkAction *action,
                 gint i)
{
	if (i == 0)
		return _("grab focus");

	return NULL;
}

static gboolean
do_action (AtkAction *action,
           gint i)
{
	GtkWidget *widget;
	gboolean return_value = TRUE;

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (action));

	if (widget == NULL) {
	/*
	* State is defunct
	*/
	return FALSE;
	}

	if (!gtk_widget_get_sensitive (widget) || !gtk_widget_get_visible (widget))
		return FALSE;

	switch (i) {
	case 0:
		gtk_widget_grab_focus (widget);
		G_GNUC_FALLTHROUGH;
	default:
		return_value = FALSE;
		break;
	}
	return return_value;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
	g_return_if_fail (iface != NULL);

	iface->do_action = do_action;
	iface->get_n_actions = get_n_actions;
	iface->get_description = get_description;
	iface->get_name = action_get_name;
}

static void
gtk_html_a11y_finalize (GObject *obj)
{
	G_OBJECT_CLASS (gtk_html_a11y_parent_class)->finalize (obj);
}

static gint
gtk_html_a11y_get_n_children (AtkObject *accessible)
{
	HTMLObject *clue;
	gint n_children = 0;
	AtkStateSet *ss;

	if (GTK_HTML_A11Y_GTKHTML (accessible)->engine->parsing)
		return 0;

	ss = atk_object_ref_state_set (accessible);
	if (atk_state_set_contains_state (ss, ATK_STATE_DEFUNCT)) {
		g_object_unref (ss);
		return 0;
	}
	g_object_unref (ss);

	clue = GTK_HTML_A11Y_GTKHTML (accessible)->engine->clue;
	if (clue) {
		AtkObject *atk_clue = html_utils_get_accessible (clue, NULL);
		if (atk_clue) {
			AtkStateSet *ss_clue = atk_object_ref_state_set (atk_clue);
			if (atk_state_set_contains_state (ss_clue, ATK_STATE_DEFUNCT)) {
				g_object_unref (ss_clue);
				return 0;
			}
			g_object_unref (ss_clue);
		}

		n_children = html_object_get_n_children (GTK_HTML_A11Y_GTKHTML (accessible)->engine->clue);
	}

	/* printf ("gtk_html_a11y_get_n_children resolves to %d\n", n_children); */

	return n_children;
}

static AtkObject *
gtk_html_a11y_ref_child (AtkObject *accessible,
                         gint index)
{
	HTMLObject *child;
	AtkObject *accessible_child = NULL;
	AtkStateSet *ss;

	if (GTK_HTML_A11Y_GTKHTML (accessible)->engine->parsing)
		return NULL;

	ss = atk_object_ref_state_set (accessible);
	if (atk_state_set_contains_state (ss, ATK_STATE_DEFUNCT)) {
		g_object_unref (ss);
		return NULL;
	}
	g_object_unref (ss);

	if (GTK_HTML_A11Y_GTKHTML (accessible)->engine->clue) {
		AtkObject *atk_clue = html_utils_get_accessible (GTK_HTML_A11Y_GTKHTML (accessible)->engine->clue, NULL);
		if (atk_clue) {
			AtkStateSet *ss_clue = atk_object_ref_state_set (atk_clue);
			if (atk_state_set_contains_state (ss_clue, ATK_STATE_DEFUNCT)) {
				g_object_unref (ss_clue);
				return NULL;
			}
			g_object_unref (ss_clue);
		}

		child = html_object_get_child (GTK_HTML_A11Y_GTKHTML (accessible)->engine->clue, index);
		if (child) {
			accessible_child = html_utils_get_accessible (child, accessible);
			if (accessible_child)
				g_object_ref (accessible_child);
		}
	}

	/* printf ("gtk_html_a11y_ref_child %d resolves to %p\n", index, accessible_child); */

	return accessible_child;
}

static const gchar *
gtk_html_a11y_get_name (AtkObject *obj)
{
	if (obj->name != NULL) {
		return obj->name;
	}

	return _("Panel containing HTML");
}

static void
gtk_html_a11y_class_init (GtkHTMLA11YClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	gtk_html_a11y_parent_class = g_type_class_peek_parent (klass);

	atk_class->initialize = gtk_html_a11y_initialize;
	atk_class->get_n_children = gtk_html_a11y_get_n_children;
	atk_class->ref_child = gtk_html_a11y_ref_child;
	atk_class->get_name = gtk_html_a11y_get_name;

	gobject_class->finalize = gtk_html_a11y_finalize;
}

static void
gtk_html_a11y_init (GtkHTMLA11Y *a11y)
{
}

static AtkObject *
gtk_html_a11y_get_focus_object (GtkWidget *widget)
{
	GtkHTML * html;
	HTMLObject * htmlobj = NULL;
	AtkObject *obj = NULL;
	gint offset;

	html = GTK_HTML (widget);

	g_return_val_if_fail (html && html->engine, NULL);

	if (!html->engine->caret_mode && !gtk_html_get_editable (html))
		htmlobj = html_engine_get_focus_object (html->engine, &offset);
	else if (html->engine && html->engine->cursor)
		htmlobj = html->engine->cursor->object;

	if (htmlobj)
		obj = html_utils_get_accessible (htmlobj, NULL);

	return obj;
}

static AtkObject * gtk_html_a11y_focus_object = NULL;

static void
gtk_html_a11y_grab_focus_cb (GtkWidget *widget)
{
	AtkObject *focus_object, *obj, *clue;

	focus_object = gtk_html_a11y_get_focus_object (widget);
	if (focus_object == NULL)
		return;

	obj = gtk_widget_get_accessible (widget);

	clue = html_utils_get_accessible (GTK_HTML (widget)->engine->clue, obj);
	atk_object_set_parent (clue, obj);

	gtk_html_a11y_focus_object = focus_object;
	atk_focus_tracker_notify (focus_object);
}

static void
gtk_html_a11y_cursor_changed_cb (GtkWidget *widget)
{
	AtkObject *focus_object;

	focus_object = gtk_html_a11y_get_focus_object (widget);
	g_return_if_fail (focus_object != NULL);

	if (gtk_html_a11y_focus_object != focus_object) {
		gtk_html_a11y_focus_object = focus_object;
		atk_focus_tracker_notify (focus_object);
	} else {
		if (G_IS_HTML_A11Y_TEXT (focus_object)) {
			gint offset;

			offset = (GTK_HTML (widget))->engine->cursor->offset;
			g_signal_emit_by_name (focus_object, "text_caret_moved",offset);
		}
	}
}

static void
gtk_html_a11y_insert_object_cb (GtkWidget *widget,
                                gint pos,
                                gint len)
{
	AtkObject * a11y;

	HTMLCursor *cursor = GTK_HTML (widget)->engine->cursor;

	a11y = gtk_html_a11y_get_focus_object (widget);
	g_return_if_fail (a11y != NULL);

	if (gtk_html_a11y_focus_object != a11y) {
		gtk_html_a11y_focus_object = a11y;
		atk_focus_tracker_notify (a11y);
	}

	if (G_IS_HTML_A11Y_TEXT (a11y)) {
		g_signal_emit_by_name (a11y, "text_changed::insert", cursor->offset - len, len);

	}
}

static void
gtk_html_a11y_delete_object_cb (GtkWidget *widget,
                                gint pos,
                                gint len)
{
	AtkObject * a11y;

	a11y = gtk_html_a11y_get_focus_object (widget);
	g_return_if_fail (a11y != NULL);

	if (gtk_html_a11y_focus_object != a11y) {
		gtk_html_a11y_focus_object = a11y;
		atk_focus_tracker_notify (a11y);
	}

	if (G_IS_HTML_A11Y_TEXT (a11y)) {
		g_signal_emit_by_name (a11y, "text_changed::delete", pos, len);
	}
}

static void
gtk_html_a11y_initialize (AtkObject *obj,
                          gpointer data)
{
	GtkWidget *widget;
	GtkHTML *html;
	AtkObject *accessible;
	AtkObject *focus_object = NULL;

	/* printf ("gtk_html_a11y_initialize\n"); */

	if (ATK_OBJECT_CLASS (gtk_html_a11y_parent_class)->initialize)
		ATK_OBJECT_CLASS (gtk_html_a11y_parent_class)->initialize (obj, data);

	g_object_set_data (G_OBJECT (obj), GTK_HTML_ID, data);

	obj->role = ATK_ROLE_PANEL;

	widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
	accessible = ATK_OBJECT (obj);

	g_signal_connect (widget, "grab_focus",
			G_CALLBACK (gtk_html_a11y_grab_focus_cb),
			NULL);
	g_signal_connect (widget, "cursor_changed",
			G_CALLBACK (gtk_html_a11y_cursor_changed_cb),
			NULL);
	g_signal_connect_after (widget, "object_inserted",
			G_CALLBACK (gtk_html_a11y_insert_object_cb),
			NULL);
	g_signal_connect_after (widget, "object_delete",
			G_CALLBACK (gtk_html_a11y_delete_object_cb),
			NULL);

	html = GTK_HTML (widget);

	if (html->engine != NULL && html->engine->clue != NULL) {
		html_utils_get_accessible (html->engine->clue, accessible);
		focus_object = gtk_html_a11y_get_focus_object (widget);
	}

	if (focus_object && gtk_html_a11y_focus_object != focus_object) {
		gtk_html_a11y_focus_object = focus_object;
		atk_focus_tracker_notify (focus_object);
	}
}
