/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   Authors:           Radek Doulik (rodo@helixcode.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlobject.h"

#include "spell.h"

#define DICTIONARY_IID "OAFIID:GNOME_Spell_Dictionary:0.1"

struct _SpellPopup {
	GtkHTMLControlData *cd;

	GtkWidget *window;
	GtkWidget *clist;

	gchar *misspeled_word;
	gboolean replace;
};
typedef struct _SpellPopup SpellPopup;

static void
destroy (GtkWidget *w, SpellPopup *sp)
{
	/* printf ("destroy popup\n"); */

	if (sp->replace) {
		gchar *replacement;

		if (GTK_CLIST (sp->clist)->selection) {
			CORBA_Environment ev;

			CORBA_exception_init (&ev);
			GNOME_Spell_Dictionary_setCorrection (sp->cd->dict, sp->misspeled_word, replacement, &ev);
			CORBA_exception_free (&ev);

			gtk_clist_get_text (GTK_CLIST (sp->clist),
					    GPOINTER_TO_INT (GTK_CLIST (sp->clist)->selection->data), 0, &replacement);
			html_engine_replace_word_with (sp->cd->html->engine, replacement);

			/* printf ("replace: %s with: %s\n", sp->misspeled_word, replacement); */
		}
	}

	gtk_grab_remove (sp->window);
	if (sp->misspeled_word)
		g_free (sp->misspeled_word);
	sp->misspeled_word = NULL;
}

static void
key_press_event (GtkWidget *widget, GdkEventKey *event, SpellPopup *sp)
{
	switch (event->keyval) {
	case GDK_space:
	case GDK_Return:
	case GDK_KP_Enter:
		sp->replace = TRUE;
	case GDK_Escape:
	case GDK_Left:
	case GDK_Right:
		gtk_widget_destroy (sp->window);
		break;
	}
}

static void
select_row (GtkCList *clist, gint row, gint column, GdkEvent *event, SpellPopup *sp)
{
	if (event == NULL)
		return;

	if (event->type == GDK_BUTTON_PRESS) {
		sp->replace = TRUE;
		gtk_widget_destroy (sp->window);
	}
}

static void
fill_suggestion_clist (GtkWidget *clist, const gchar *word, GtkHTMLControlData *cd)
{
	GNOME_Spell_StringSeq *seq;
	CORBA_Environment      ev;
	const char * suggested_word [1];
	gint i;

	CORBA_exception_init (&ev);
	seq = GNOME_Spell_Dictionary_getSuggestions (cd->dict, word, &ev );

	if (ev._major == CORBA_NO_EXCEPTION) {
		for (i=0; i < seq->_length; i++) {
			suggested_word [0] = seq->_buffer [i];
			gtk_clist_append (GTK_CLIST (clist), (gchar **) suggested_word);
		}
		CORBA_free (seq);
	}
	CORBA_exception_free (&ev);
}

void
spell_suggestion_request (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd;
	SpellPopup *sp;
	HTMLEngine *e = html->engine;
	GtkWidget *scrolled_window;
	GtkWidget *frame;
	gint x, y, xw, yw;

	/* printf ("spell_suggestion_request_cb %s\n", word); */

	cd = (GtkHTMLControlData *) data;
	sp = g_new (SpellPopup, 1);
	sp->cd = cd;
	sp->replace = FALSE;
	sp->misspeled_word = g_strdup (word);

	sp->window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_policy (GTK_WINDOW (sp->window), FALSE, FALSE, FALSE);
	gtk_widget_set_name (sp->window, "html-editor-spell-suggestions");
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	sp->clist  = gtk_clist_new (1);
	gtk_clist_set_selection_mode (GTK_CLIST (sp->clist), GTK_SELECTION_BROWSE);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

	fill_suggestion_clist (sp->clist, word, cd);

	gtk_widget_set_usize (sp->window, gtk_clist_columns_autosize (GTK_CLIST (sp->clist)) + 40, 200);
	gtk_container_add (GTK_CONTAINER (scrolled_window), sp->clist);
	gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
	gtk_container_add (GTK_CONTAINER (sp->window), frame);
	gtk_widget_show_all (frame);

	gdk_window_get_origin (GTK_WIDGET (html)->window, &xw, &yw);
	html_object_get_cursor_base (e->cursor->object, e->painter, e->cursor->offset, &x, &y);
	/* printf ("x: %d y: %d\n", x, y); */
	x += xw + e->leftBorder + 5;
	y += yw + e->cursor->object->ascent + e->cursor->object->descent + e->topBorder + 4;
	/* printf ("x: %d y: %d\n", x, y); */

	gtk_signal_connect (GTK_OBJECT (sp->window),   "key_press_event", key_press_event, sp);
	gtk_signal_connect (GTK_OBJECT (sp->window),   "destroy",         destroy, sp);
	gtk_signal_connect (GTK_OBJECT (sp->clist),    "select_row",      select_row, sp);

	gtk_widget_popup (sp->window, x, y);
	gtk_grab_add (sp->window);
	gtk_window_set_focus (GTK_WINDOW (sp->window), sp->clist);
}

BonoboObjectClient *
spell_new_dictionary (void)
{
	BonoboObjectClient *dictionary_client = bonobo_object_activate (DICTIONARY_IID, 0);

	if (!dictionary_client) {
		g_warning ("Cannot activate spell dictionary (iid:%s)", DICTIONARY_IID);
		return NULL;
	}

	return dictionary_client;
}

gboolean
spell_check_word (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;
	gboolean rv;

	if (!cd->dict)
		return TRUE;

	CORBA_exception_init (&ev);
	rv = GNOME_Spell_Dictionary_checkWord (cd->dict, word, &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		rv = TRUE;
	CORBA_exception_free (&ev);

	return rv;
}

void
spell_add_to_session (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;

	if (!cd->dict)
		return;

	CORBA_exception_init (&ev);
	GNOME_Spell_Dictionary_addWordToSession (cd->dict, word, &ev);
	CORBA_exception_free (&ev);
}

void
spell_add_to_personal (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;

	if (!cd->dict)
		return;

	CORBA_exception_init (&ev);
	GNOME_Spell_Dictionary_addWordToPersonal (cd->dict, word, &ev);
	CORBA_exception_free (&ev);
}
