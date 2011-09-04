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

#ifndef GTKHTML_IMAGE_CHOOSER_DIALOG_H
#define GTKHTML_IMAGE_CHOOSER_DIALOG_H

#include "gtkhtml-editor-common.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG \
	(gtkhtml_image_chooser_dialog_get_type ())
#define GTKHTML_IMAGE_CHOOSER_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG, GtkhtmlImageChooserDialog))
#define GTKHTML_IMAGE_CHOOSER_DIALOG_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG, GtkhtmlImageChooserDialogClass))
#define GTKHTML_IS_IMAGE_CHOOSER_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG))
#define GTKHTML_IS_IMAGE_CHOOSER_DIALOG_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG))
#define GTKHTML_IMAGE_CHOOSER_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG, GtkhtmlImageChooserDialogClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlImageChooserDialog GtkhtmlImageChooserDialog;
typedef struct _GtkhtmlImageChooserDialogClass GtkhtmlImageChooserDialogClass;
typedef struct _GtkhtmlImageChooserDialogPrivate GtkhtmlImageChooserDialogPrivate;

struct _GtkhtmlImageChooserDialog {
	GtkFileChooserDialog parent;
	GtkhtmlImageChooserDialogPrivate *priv;
};

struct _GtkhtmlImageChooserDialogClass {
	GtkFileChooserDialogClass parent_class;
};

GType		gtkhtml_image_chooser_dialog_get_type
					(void) G_GNUC_CONST;
GtkWidget *	gtkhtml_image_chooser_dialog_new
					(const gchar *title,
					 GtkWindow *parent);
GFile *		gtkhtml_image_chooser_dialog_run
					(GtkhtmlImageChooserDialog *dialog);

G_END_DECLS

#endif /* GTKHTML_IMAGE_CHOOSER_DIALOG_H */
