/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-spell-checker.c
 *
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2025 yetist <yetist@gmail.com>
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

#include "gtkhtml-spell-checker.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <enchant.h>

#include "gtkhtml-spell-marshal.h"

enum {
	PROP_0,
	PROP_LANGUAGE
};

enum {
	ADDED,
	ADDED_TO_SESSION,
	SESSION_CLEARED,
	LAST_SIGNAL
};

struct _GtkhtmlSpellCheckerPrivate {
	EnchantDict *dict;
	EnchantBroker *broker;
	const GtkhtmlSpellLanguage *language;
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE (GtkhtmlSpellChecker, gtkhtml_spell_checker, G_TYPE_OBJECT);
static gboolean
spell_checker_is_digit (const gchar *text,
                        gssize length)
{
	const gchar *cp;
	const gchar *end;

	g_return_val_if_fail (text != NULL, FALSE);

	if (length < 0)
		length = strlen (text);

	cp = text;
	end = text + length;

	while (cp != end) {
		gunichar c;

		c = g_utf8_get_char (cp);

		if (!g_unichar_isdigit (c) && c != '.' && c != ',')
			return FALSE;

		cp = g_utf8_next_char (cp);
	}

	return TRUE;
}

static EnchantDict *
spell_checker_request_dict (GtkhtmlSpellChecker *checker)
{
	GtkhtmlSpellCheckerPrivate *priv;
	const gchar *code;

	/* Loading a dictionary is time-consuming, so delay it until we
	 * really need it.  The assumption being that the dictionary
	 * for a particular GtkhtmlSpellChecker instance will only
	 * occasionally be needed.  That way we can create as many
	 * instances as we want without a huge performance penalty. */

	priv = gtkhtml_spell_checker_get_instance_private (checker);

	if (priv->dict != NULL)
		return priv->dict;

	if (priv->language == NULL)
		return NULL;

	code = gtkhtml_spell_language_get_code (priv->language);
	priv->dict = enchant_broker_request_dict (priv->broker, code);

	if (priv->dict == NULL) {
		priv->language = NULL;
		g_warning ("Cannot load the dictionary for %s", code);
	}

	return priv->dict;
}

static GObject *
spell_checker_constructor (GType type,
                           guint n_construct_properties,
                           GObjectConstructParam *construct_properties)
{
	GObject *object;

	/* Chain up to parent's constructor() method. */
	object = G_OBJECT_CLASS (gtkhtml_spell_checker_parent_class)->constructor (
		type, n_construct_properties, construct_properties);

	gtkhtml_spell_checker_clear_session (GTKHTML_SPELL_CHECKER (object));

	return object;
}

static void
spell_checker_set_property (GObject *object,
                            guint property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
	GtkhtmlSpellCheckerPrivate *priv;
	GtkhtmlSpellChecker *checker;

	checker = GTKHTML_SPELL_CHECKER (object);
	priv = gtkhtml_spell_checker_get_instance_private (checker);

	switch (property_id) {
		case PROP_LANGUAGE:
			priv->language = g_value_get_boxed (value);
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
spell_checker_get_property (GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_LANGUAGE:
			g_value_set_boxed (
				value, gtkhtml_spell_checker_get_language (
				GTKHTML_SPELL_CHECKER (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
spell_checker_finalize (GObject *object)
{
	GtkhtmlSpellCheckerPrivate *priv;
	GtkhtmlSpellChecker *checker;

	checker = GTKHTML_SPELL_CHECKER (object);
	priv = gtkhtml_spell_checker_get_instance_private (checker);

	if (priv->dict != NULL)
		enchant_broker_free_dict (priv->broker, priv->dict);
	enchant_broker_free (priv->broker);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (gtkhtml_spell_checker_parent_class)->finalize (object);
}

static void
gtkhtml_spell_checker_class_init (GtkhtmlSpellCheckerClass *klass)
{
	GObjectClass *object_class;

	gtkhtml_spell_checker_parent_class = g_type_class_peek_parent (klass);

	object_class = G_OBJECT_CLASS (klass);
	object_class->constructor = spell_checker_constructor;
	object_class->set_property = spell_checker_set_property;
	object_class->get_property = spell_checker_get_property;
	object_class->finalize = spell_checker_finalize;

	g_object_class_install_property (
		object_class,
		PROP_LANGUAGE,
		g_param_spec_boxed (
			"language",
			"Language",
			"The language used by the spell checker",
			GTKHTML_TYPE_SPELL_LANGUAGE,
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_READWRITE));

	signals[ADDED] = g_signal_new (
		"added",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (
		GtkhtmlSpellCheckerClass, added),
		NULL, NULL,
		gtkhtml_spell_marshal_VOID__STRING_INT,
		G_TYPE_NONE, 2,
		G_TYPE_STRING,
		G_TYPE_INT);

	signals[ADDED_TO_SESSION] = g_signal_new (
		"added-to-session",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (
		GtkhtmlSpellCheckerClass, added_to_session),
		NULL, NULL,
		gtkhtml_spell_marshal_VOID__STRING_INT,
		G_TYPE_NONE, 2,
		G_TYPE_STRING,
		G_TYPE_INT);

	signals[SESSION_CLEARED] = g_signal_new (
		"session-cleared",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (
		GtkhtmlSpellCheckerClass, session_cleared),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
gtkhtml_spell_checker_init (GtkhtmlSpellChecker *checker)
{
	GtkhtmlSpellCheckerPrivate *priv;

	priv = gtkhtml_spell_checker_get_instance_private (checker);
	priv->broker = enchant_broker_init ();
}

GtkhtmlSpellChecker *
gtkhtml_spell_checker_new (const GtkhtmlSpellLanguage *language)
{
	return g_object_new (
		GTKHTML_TYPE_SPELL_CHECKER, "language", language, NULL);
}

const GtkhtmlSpellLanguage *
gtkhtml_spell_checker_get_language (GtkhtmlSpellChecker *checker)
{
	GtkhtmlSpellCheckerPrivate *priv;

	g_return_val_if_fail (GTKHTML_IS_SPELL_CHECKER (checker), NULL);
	priv = gtkhtml_spell_checker_get_instance_private (checker);

	return priv->language;
}

gboolean
gtkhtml_spell_checker_check_word (GtkhtmlSpellChecker *checker,
                                  const gchar *word,
                                  gssize length)
{
	EnchantDict *dict;
	gint result;

	g_return_val_if_fail (GTKHTML_IS_SPELL_CHECKER (checker), FALSE);
	g_return_val_if_fail (word != NULL, FALSE);

	if ((dict = spell_checker_request_dict (checker)) == NULL)
		return FALSE;

	if (length < 0)
		length = strlen (word);

	if (spell_checker_is_digit (word, length))
		return TRUE;

	/* Exclude apostrophies from the end of words. */
	while (word[length - 1] == '\'')
		length--;

	result = enchant_dict_check (dict, word, length);

	if (result < 0)
		g_warning (
			"Error checking word '%s' (%s)",
			word, enchant_dict_get_error (dict));

	return (result == 0);
}

GList *
gtkhtml_spell_checker_get_suggestions (GtkhtmlSpellChecker *checker,
                                       const gchar *word,
                                       gssize length)
{
	EnchantDict *dict;
	gchar **suggestions;
	gsize n_suggestions = 0;
	GList *list = NULL;

	g_return_val_if_fail (GTKHTML_IS_SPELL_CHECKER (checker), NULL);

	if ((dict = spell_checker_request_dict (checker)) == NULL)
		return NULL;

	suggestions = enchant_dict_suggest (
		dict, word, length, &n_suggestions);

	while (n_suggestions > 0)
		list = g_list_prepend (list, suggestions[--n_suggestions]);

	g_free (suggestions);

	return list;
}

void
gtkhtml_spell_checker_store_replacement (GtkhtmlSpellChecker *checker,
                                         const gchar *word,
                                         gssize word_length,
                                         const gchar *replacement,
                                         gssize replacement_length)
{
	EnchantDict *dict;

	g_return_if_fail (GTKHTML_IS_SPELL_CHECKER (checker));

	if ((dict = spell_checker_request_dict (checker)) == NULL)
		return;

	enchant_dict_store_replacement (
		dict, word, word_length, replacement, replacement_length);
}

void
gtkhtml_spell_checker_add_word (GtkhtmlSpellChecker *checker,
                                const gchar *word,
                                gssize length)
{
	EnchantDict *dict;

	g_return_if_fail (GTKHTML_IS_SPELL_CHECKER (checker));

	if ((dict = spell_checker_request_dict (checker)) == NULL)
		return;

	enchant_dict_add (dict, word, length);
	g_signal_emit (G_OBJECT (checker), signals[ADDED], 0, word, length);
}

void
gtkhtml_spell_checker_add_word_to_session (GtkhtmlSpellChecker *checker,
                                           const gchar *word,
                                           gssize length)
{
	EnchantDict *dict;

	g_return_if_fail (GTKHTML_IS_SPELL_CHECKER (checker));

	if ((dict = spell_checker_request_dict (checker)) == NULL)
		return;

	enchant_dict_add_to_session (dict, word, length);
	g_signal_emit (G_OBJECT (checker), signals[ADDED_TO_SESSION], 0, word, length);
}

void
gtkhtml_spell_checker_clear_session (GtkhtmlSpellChecker *checker)
{
	GtkhtmlSpellCheckerPrivate *priv;

	g_return_if_fail (GTKHTML_IS_SPELL_CHECKER (checker));

	priv = gtkhtml_spell_checker_get_instance_private (checker);
	if (priv->dict != NULL) {
		enchant_broker_free_dict (priv->broker, priv->dict);
		priv->dict = NULL;
	}

	if (priv->language == NULL)
		priv->language = gtkhtml_spell_language_lookup (NULL);

	g_signal_emit (G_OBJECT (checker), signals[SESSION_CLEARED], 0);
}

gint
gtkhtml_spell_checker_compare (GtkhtmlSpellChecker *checker_a,
                               GtkhtmlSpellChecker *checker_b)
{
	const GtkhtmlSpellLanguage *language_a;
	const GtkhtmlSpellLanguage *language_b;

	language_a = gtkhtml_spell_checker_get_language (checker_a);
	language_b = gtkhtml_spell_checker_get_language (checker_b);

	return gtkhtml_spell_language_compare (language_a, language_b);
}
