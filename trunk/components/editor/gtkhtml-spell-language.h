/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-spell-language.h
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

/* Based on Marco Barisione's GSpellLanguage. */

#ifndef GTKHTML_SPELL_LANGUAGE_H
#define GTKHTML_SPELL_LANGUAGE_H

#include <gtkhtml-editor-common.h>

G_BEGIN_DECLS

typedef struct _GtkhtmlSpellLanguage GtkhtmlSpellLanguage;

#define GTKHTML_TYPE_SPELL_LANGUAGE \
	(gtkhtml_spell_language_get_type ())

GType		gtkhtml_spell_language_get_type		(void);
const GList *	gtkhtml_spell_language_get_available	(void);

const GtkhtmlSpellLanguage *
		gtkhtml_spell_language_lookup
					(const gchar *language_code);
const gchar *	gtkhtml_spell_language_get_code
					(const GtkhtmlSpellLanguage *language);
const gchar *	gtkhtml_spell_language_get_name
					(const GtkhtmlSpellLanguage *language);
gint		gtkhtml_spell_language_compare
					(const GtkhtmlSpellLanguage *language_a,
					 const GtkhtmlSpellLanguage *language_b);

G_END_DECLS

#endif /* GTKHTML_SPELL_LANGUAGE_H */
