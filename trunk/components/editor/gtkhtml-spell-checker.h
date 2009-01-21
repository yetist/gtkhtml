/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-spell-checker.h
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

/* Based on Marco Barisione's GSpellChecker. */

#ifndef GTKHTML_SPELL_CHECKER_H
#define GTKHTML_SPELL_CHECKER_H

#include <gtkhtml-editor-common.h>
#include <gtkhtml-spell-language.h>

/* Standard GObject macros */
#define GTKHTML_TYPE_SPELL_CHECKER \
	(gtkhtml_spell_checker_get_type ())
#define GTKHTML_SPELL_CHECKER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_SPELL_CHECKER, GtkhtmlSpellChecker))
#define GTKHTML_SPELL_CHECKER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_SPELL_CHECKER, GtkhtmlSpellCheckerClass))
#define GTKHTML_IS_SPELL_CHECKER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_SPELL_CHECKER))
#define GTKHTML_IS_SPELL_CHECKER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_SPELL_CHECKER))
#define GTKHTML_SPELL_CHECKER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_SPELL_CHECKER, GtkhtmlSpellCheckerClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlSpellChecker GtkhtmlSpellChecker;
typedef struct _GtkhtmlSpellCheckerClass GtkhtmlSpellCheckerClass;
typedef struct _GtkhtmlSpellCheckerPrivate GtkhtmlSpellCheckerPrivate;

struct _GtkhtmlSpellChecker {
	GObject parent;
	GtkhtmlSpellCheckerPrivate *priv;
};

struct _GtkhtmlSpellCheckerClass {
	GObjectClass parent;

	void	(*added)		(GtkhtmlSpellChecker *checker,
					 const gchar *word,
					 gint length);
	void	(*added_to_session)	(GtkhtmlSpellChecker *checker,
					 const gchar *word,
					 gint length);
	void	(*session_cleared)	(GtkhtmlSpellChecker *checker);
};

GType		gtkhtml_spell_checker_get_type	(void);
GtkhtmlSpellChecker *
		gtkhtml_spell_checker_new
					(const GtkhtmlSpellLanguage *language);
const GtkhtmlSpellLanguage *
		gtkhtml_spell_checker_get_language
					(GtkhtmlSpellChecker *checker);
gboolean	gtkhtml_spell_checker_check_word
					(GtkhtmlSpellChecker *checker,
					 const gchar *word,
					 gssize length);
GList *		gtkhtml_spell_checker_get_suggestions
					(GtkhtmlSpellChecker *checker,
					 const gchar *word,
					 gssize length);
void		gtkhtml_spell_checker_store_replacement
					(GtkhtmlSpellChecker *checker,
					 const gchar *word,
					 gssize word_length,
					 const gchar *replacement,
					 gssize replacement_length);
void		gtkhtml_spell_checker_add_word
					(GtkhtmlSpellChecker *checker,
					 const gchar *word,
					 gssize length);
void		gtkhtml_spell_checker_add_word_to_session
					(GtkhtmlSpellChecker *checker,
					 const gchar *word,
					 gssize length);
void		gtkhtml_spell_checker_clear_session
					(GtkhtmlSpellChecker *checker);
gint		gtkhtml_spell_checker_compare
					(GtkhtmlSpellChecker *checker_a,
					 GtkhtmlSpellChecker *checker_b);

G_END_DECLS

#endif /* GTKHTML_SPELL_CHECKER_H */
