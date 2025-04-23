/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-spell-dialog.h
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

#pragma once

#include "gtkhtml-spell-checker.h"

G_BEGIN_DECLS

#define GTKHTML_TYPE_SPELL_DIALOG              (gtkhtml_spell_dialog_get_type ())

G_DECLARE_FINAL_TYPE (GtkhtmlSpellDialog, gtkhtml_spell_dialog, GTKHTML, SPELL_DIALOG, GtkDialog)

GtkWidget *	gtkhtml_spell_dialog_new	(GtkWindow *parent);
void		gtkhtml_spell_dialog_close	(GtkhtmlSpellDialog *dialog);
const gchar *	gtkhtml_spell_dialog_get_word	(GtkhtmlSpellDialog *dialog);
void		gtkhtml_spell_dialog_set_word	(GtkhtmlSpellDialog *dialog,
						 const gchar *word);
void		gtkhtml_spell_dialog_next_word	(GtkhtmlSpellDialog *dialog);
void		gtkhtml_spell_dialog_prev_word	(GtkhtmlSpellDialog *dialog);
GList *		gtkhtml_spell_dialog_get_spell_checkers
						(GtkhtmlSpellDialog *dialog);
void		gtkhtml_spell_dialog_set_spell_checkers
						(GtkhtmlSpellDialog *dialog,
						 GList *spell_checkers);
GtkhtmlSpellChecker *
		gtkhtml_spell_dialog_get_active_checker
						(GtkhtmlSpellDialog *dialog);
gchar *		gtkhtml_spell_dialog_get_active_suggestion
						(GtkhtmlSpellDialog *dialog);

G_END_DECLS
