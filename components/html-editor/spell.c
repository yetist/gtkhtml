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

#include <config.h>
#include <glib.h>
#include <libgnome/gnome-i18n.h>

#include "gtkhtml.h"
#include "gtkhtml-properties.h"

#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-selection-updater.h"
#include "htmlobject.h"
#include "htmlselection.h"

#include "menubar.h"
#include "spell.h"

#define CONTROL_IID "OAFIID:GNOME_Spell_Control:0.3"

void
spell_suggestion_request (GtkHTML *html, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;

	spell_check_dialog (cd, FALSE);
}

GNOME_Spell_Dictionary
spell_new_dictionary (void)
{
#define DICTIONARY_IID "OAFIID:GNOME_Spell_Dictionary:0.3"

	GNOME_Spell_Dictionary dictionary = bonobo_get_object (DICTIONARY_IID, "GNOME/Spell/Dictionary", NULL);

	if (dictionary == CORBA_OBJECT_NIL) {
		g_warning ("Cannot create spell dictionary instance (iid:%s)", DICTIONARY_IID);
	}
#undef DICTIONARY_IID

	return dictionary;
}

gboolean
spell_check_word (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;
	gboolean rv;

	if (!cd->dict)
		return TRUE;

	/* printf ("check word: %s\n", word); */
	CORBA_exception_init (&ev);
	rv = GNOME_Spell_Dictionary_checkWord (cd->dict, word, &ev);
	if (ev._major == CORBA_SYSTEM_EXCEPTION)
		rv = TRUE;
	CORBA_exception_free (&ev);

	return rv;
}

void
spell_add_to_session (GtkHTML *html, const gchar *word, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;

	g_return_if_fail (word);

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

	g_return_if_fail (word);

	if (!cd->dict)
		return;

	CORBA_exception_init (&ev);
	/* FIXME spell GNOME_Spell_Dictionary_addWordToPersonal (cd->dict, word, &ev); */
	CORBA_exception_free (&ev);
}

void
spell_set_language (GtkHTML *html, const gchar *language, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) data;
	CORBA_Environment   ev;

	/* printf ("spell_set_language\n"); */
	if (!cd->dict)
		return;

	/* printf ("spell_set_language '%s'\n", language); */

	CORBA_exception_init (&ev);
	GNOME_Spell_Dictionary_setLanguage (cd->dict, language, &ev);
	CORBA_exception_free (&ev);

	menubar_set_languages (cd, language);
}

void
spell_init (GtkHTML *html, GtkHTMLControlData *cd)
{
}

static void
set_word (GtkHTMLControlData *cd)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	html_engine_select_spell_word_editable (cd->html->engine);
	bonobo_property_bag_client_set_value_string (cd->spell_control_pb, "word",
						     html_engine_get_spell_word (cd->html->engine), &ev);
	CORBA_exception_free (&ev);
}

static gboolean
next_word (GtkHTMLControlData *cd)
{
	gboolean rv = TRUE;
	while (html_engine_forward_word (cd->html->engine)
	       && (rv = html_engine_spell_word_is_valid (cd->html->engine)))
		;

	return rv;
}

static void
check_next_word (GtkHTMLControlData *cd, gboolean update)
{
	HTMLEngine *e = cd->html->engine;

	html_engine_disable_selection (e);
	if (update)
		html_engine_spell_check (e);

	if (!cd->spell_check_next || next_word (cd)) {
		gtk_dialog_response (GTK_DIALOG (cd->spell_dialog), GTK_RESPONSE_CLOSE);
	} else {
		set_word (cd);
	}
}

static void 
replace_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;

	html_engine_replace_spell_word_with (cd->html->engine, BONOBO_ARG_GET_STRING (arg));
	check_next_word (cd, FALSE);
}

static void 
skip_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	check_next_word ((GtkHTMLControlData *) user_data, FALSE);
}

static void 
add_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;
	gchar *word;

	word = html_engine_get_spell_word (cd->html->engine);
	g_return_if_fail (word);

	/* FIXME spell GNOME_Spell_Dictionary_addWordToPersonal (cd->dict, word, ev); */
	g_free (word);
	check_next_word ((GtkHTMLControlData *) user_data, TRUE);
}

static void 
ignore_cb (BonoboListener *listener, const char *event_name, const CORBA_any *arg, CORBA_Environment *ev, gpointer user_data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *) user_data;
	gchar *word;

	word = html_engine_get_spell_word (cd->html->engine);
	g_return_if_fail (word);

	GNOME_Spell_Dictionary_addWordToSession (cd->dict, word, ev);
	g_free (word);
	check_next_word ((GtkHTMLControlData *) user_data, TRUE);
}

gboolean
spell_has_control ()
{
	GtkWidget *control;
	gboolean rv;

	control = bonobo_widget_new_control (CONTROL_IID, CORBA_OBJECT_NIL);
	rv = control != NULL;

	if (control) {
		gtk_object_sink (GTK_OBJECT (control));
	}
	return rv;
}

void
spell_check_dialog (GtkHTMLControlData *cd, gboolean whole_document)
{
	GtkWidget *control;
	GtkWidget *dialog;
	guint position;

	position = cd->html->engine->cursor->position;
	cd->spell_check_next = whole_document;
	if (whole_document) {
		html_engine_disable_selection (cd->html->engine);
		html_engine_beginning_of_document (cd->html->engine);
	}

	if (html_engine_spell_word_is_valid (cd->html->engine))
		if (next_word (cd)) {
			html_engine_hide_cursor (cd->html->engine);
			html_cursor_jump_to_position (cd->html->engine->cursor, cd->html->engine, position);
			html_engine_show_cursor (cd->html->engine);

			/* FIX2 gnome_ok_dialog (_("No misspelled word found")); */

			return;
		}

	dialog  = gtk_dialog_new_with_buttons (_("Spell checker"), NULL, 0, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	control = bonobo_widget_new_control (CONTROL_IID, CORBA_OBJECT_NIL);

	if (!control) {
		g_warning ("Cannot create spell control");
		gtk_widget_unref (dialog);
		return;
	}

	cd->spell_dialog = dialog;
        cd->spell_control_pb = bonobo_control_frame_get_control_property_bag
		(bonobo_widget_get_control_frame (BONOBO_WIDGET (control)), NULL);
	bonobo_property_bag_client_set_value_string (cd->spell_control_pb, "language",
						     GTK_HTML_CLASS (GTK_OBJECT_GET_CLASS (cd->html))->properties->language,
						     NULL);

	bonobo_event_source_client_add_listener (cd->spell_control_pb, replace_cb, "Bonobo/Property:change:replace", NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, add_cb, "Bonobo/Property:change:add", NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, ignore_cb, "Bonobo/Property:change:ignore",
						 NULL, cd);
	bonobo_event_source_client_add_listener (cd->spell_control_pb, skip_cb, "Bonobo/Property:change:skip", NULL, cd);
	set_word (cd);

	gtk_widget_show (control);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), control);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	bonobo_object_release_unref (cd->spell_control_pb, NULL);
	cd->spell_control_pb = CORBA_OBJECT_NIL;
}
