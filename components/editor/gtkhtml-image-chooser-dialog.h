/*
 * gtkhtml-image-chooser-dialog.h
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG              (gtkhtml_image_chooser_dialog_get_type ())

G_DECLARE_FINAL_TYPE (GtkhtmlImageChooserDialog, gtkhtml_image_chooser_dialog, GTKHTML, IMAGE_CHOOSER_DIALOG, GtkFileChooserDialog)

GtkWidget *	gtkhtml_image_chooser_dialog_new
					(const gchar *title,
					 GtkWindow *parent);
GFile *		gtkhtml_image_chooser_dialog_run
					(GtkhtmlImageChooserDialog *dialog);

G_END_DECLS
