/*
 * gtkhtml-image-chooser-dialog.c
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

#include "gtkhtml-image-chooser-dialog.h"

#define PREVIEW_WIDTH	256
#define PREVIEW_HEIGHT	256

typedef struct _Context Context;

struct _GtkhtmlImageChooserDialogPrivate {
	GCancellable *cancellable;
	gchar *current_folder;
};

struct _Context {
	GtkFileChooser *file_chooser;
	GCancellable *cancellable;
};

enum {
	PROP_0,
	PROP_CURRENT_FOLDER
};

G_DEFINE_TYPE (
	GtkhtmlImageChooserDialog,
	gtkhtml_image_chooser_dialog,
	GTK_TYPE_FILE_CHOOSER_DIALOG)

static void
context_free (Context *context)
{
	g_object_unref (context->file_chooser);
	g_object_unref (context->cancellable);

	g_slice_free (Context, context);
}

static void
image_chooser_dialog_read_cb (GFile *preview_file,
                              GAsyncResult *result,
                              Context *context)
{
	GdkPixbuf *pixbuf;
	GtkWidget *preview_widget;
	GFileInputStream *input_stream;

	input_stream = g_file_read_finish (preview_file, result, NULL);

	/* FIXME Handle errors better, but remember
	 *       to ignore G_IO_ERROR_CANCELLED. */
	if (input_stream == NULL)
		goto exit;

	/* XXX This blocks, but GDK-PixBuf offers no asynchronous
	 *     alternative and I don't want to deal with making GDK
	 *     calls from threads and all the crap that goes with it. */
	pixbuf = gdk_pixbuf_new_from_stream_at_scale (
		G_INPUT_STREAM (input_stream),
		PREVIEW_WIDTH, PREVIEW_HEIGHT, TRUE,
		context->cancellable, NULL);

	preview_widget = gtk_file_chooser_get_preview_widget (
		context->file_chooser);

	gtk_file_chooser_set_preview_widget_active (
		context->file_chooser, pixbuf != NULL);

	gtk_image_set_from_pixbuf (GTK_IMAGE (preview_widget), pixbuf);

	if (pixbuf != NULL)
		g_object_unref (pixbuf);

	g_object_unref (input_stream);

exit:
	context_free (context);
}

static void
image_chooser_dialog_update_preview (GtkFileChooser *file_chooser)
{
	GtkhtmlImageChooserDialogPrivate *priv;
	GtkWidget *preview_widget;
	GFile *preview_file;
	Context *context;

	priv = GTKHTML_IMAGE_CHOOSER_DIALOG (file_chooser)->priv;
	preview_file = gtk_file_chooser_get_preview_file (file_chooser);
	preview_widget = gtk_file_chooser_get_preview_widget (file_chooser);

	if (priv->cancellable != NULL) {
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
		priv->cancellable = NULL;
	}

	gtk_image_clear (GTK_IMAGE (preview_widget));
	gtk_file_chooser_set_preview_widget_active (file_chooser, FALSE);

	if (preview_file == NULL)
		return;

	priv->cancellable = g_cancellable_new ();

	context = g_slice_new0 (Context);
	context->file_chooser = g_object_ref (file_chooser);
	context->cancellable = g_object_ref (priv->cancellable);

	g_file_read_async (
		preview_file, G_PRIORITY_LOW,
		priv->cancellable, (GAsyncReadyCallback)
		image_chooser_dialog_read_cb, context);

	g_object_unref (preview_file);
}

static void
image_chooser_dialog_set_property (GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_CURRENT_FOLDER:
			gtkhtml_image_chooser_dialog_set_current_folder (
				GTKHTML_IMAGE_CHOOSER_DIALOG (object),
				g_value_get_string (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
image_chooser_dialog_get_property (GObject *object,
                                   guint property_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_CURRENT_FOLDER:
			g_value_set_string (
				value,
				gtkhtml_image_chooser_dialog_get_current_folder (
				GTKHTML_IMAGE_CHOOSER_DIALOG (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
image_chooser_dialog_dispose (GObject *object)
{
	GtkhtmlImageChooserDialogPrivate *priv;

	priv = GTKHTML_IMAGE_CHOOSER_DIALOG (object)->priv;

	if (priv->cancellable != NULL) {
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
		priv->cancellable = NULL;
	}

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (gtkhtml_image_chooser_dialog_parent_class)->
		dispose (object);
}

static void
image_chooser_dialog_finalize (GObject *object)
{
	GtkhtmlImageChooserDialogPrivate *priv;

	priv = GTKHTML_IMAGE_CHOOSER_DIALOG (object)->priv;

	g_free (priv->current_folder);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (gtkhtml_image_chooser_dialog_parent_class)->
		finalize (object);
}

static void
image_chooser_dialog_constructed (GObject *object)
{
	GtkFileChooser *file_chooser;
	GtkFileFilter *file_filter;

	file_chooser = GTK_FILE_CHOOSER (object);
	gtk_file_chooser_set_local_only (file_chooser, FALSE);

	gtk_dialog_add_button (
		GTK_DIALOG (file_chooser),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (
		GTK_DIALOG (file_chooser),
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response (
		GTK_DIALOG (file_chooser), GTK_RESPONSE_ACCEPT);

	file_filter = gtk_file_filter_new ();
	gtk_file_filter_add_pixbuf_formats (file_filter);
	gtk_file_chooser_set_filter (file_chooser, file_filter);

	gtk_file_chooser_set_preview_widget (file_chooser, gtk_image_new ());
}

static void
gtkhtml_image_chooser_dialog_class_init (GtkhtmlImageChooserDialogClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (
		class, sizeof (GtkhtmlImageChooserDialogPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = image_chooser_dialog_set_property;
	object_class->get_property = image_chooser_dialog_get_property;
	object_class->dispose = image_chooser_dialog_dispose;
	object_class->finalize = image_chooser_dialog_finalize;
	object_class->constructed = image_chooser_dialog_constructed;

	g_object_class_install_property (
		object_class,
		PROP_CURRENT_FOLDER,
		g_param_spec_string (
			"current-folder",
			"Current Folder",
			"The initial folder for file chooser dialogs",
			g_get_home_dir (),
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT));
}

static void
gtkhtml_image_chooser_dialog_init (GtkhtmlImageChooserDialog *dialog)
{
	dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (
		dialog, GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG,
		GtkhtmlImageChooserDialogPrivate);

	g_signal_connect (
		dialog, "update-preview",
		G_CALLBACK (image_chooser_dialog_update_preview), NULL);
}

GtkWidget *
gtkhtml_image_chooser_dialog_new (const gchar *title,
                                  GtkWindow *parent)
{
	return g_object_new (
		GTKHTML_TYPE_IMAGE_CHOOSER_DIALOG,
		"action", GTK_FILE_CHOOSER_ACTION_OPEN,
		"title", title,
		"transient-for", parent, NULL);
}

GFile *
gtkhtml_image_chooser_dialog_run (GtkhtmlImageChooserDialog *dialog)
{
	GtkFileChooser *file_chooser;
	gchar *uri;

	g_return_val_if_fail (GTKHTML_IS_IMAGE_CHOOSER_DIALOG (dialog), NULL);

	file_chooser = GTK_FILE_CHOOSER (dialog);

	uri = (gchar *)
		gtkhtml_image_chooser_dialog_get_current_folder (dialog);
	if (uri != NULL)
		gtk_file_chooser_set_current_folder_uri (file_chooser, uri);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT)
		return NULL;

	uri = gtk_file_chooser_get_current_folder_uri (file_chooser);
	gtkhtml_image_chooser_dialog_set_current_folder (dialog, uri);
	g_free (uri);

	return gtk_file_chooser_get_file (file_chooser);
}

const gchar *
gtkhtml_image_chooser_dialog_get_current_folder (GtkhtmlImageChooserDialog *dialog)
{
	g_return_val_if_fail (GTKHTML_IS_IMAGE_CHOOSER_DIALOG (dialog), NULL);

	return dialog->priv->current_folder;
}

void
gtkhtml_image_chooser_dialog_set_current_folder (GtkhtmlImageChooserDialog *dialog,
                                                 const gchar *current_folder)
{
	g_return_if_fail (GTKHTML_IS_IMAGE_CHOOSER_DIALOG (dialog));

	if (current_folder == NULL)
		current_folder = g_get_home_dir ();

	g_free (dialog->priv->current_folder);
	dialog->priv->current_folder = g_strdup (current_folder);

	g_object_notify (G_OBJECT (dialog), "current-folder");
}
