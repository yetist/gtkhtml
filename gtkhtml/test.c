/* -*- mode: c; c-basic-offset: 8 -*- */

/*
 *  This file is part of the GuileRepl library
 *
 *  Copyright 2001 Ariel Rios <ariel@linuxppc.org>
 *  Copyright 2025 Xiaotian Wu <yetist@gmail.com>
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
 *
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gtkhtml.h"
#include "gtkhtmldebug.h"
#include "gtkhtml-stream.h"

#include "htmlengine.h"

typedef struct _Example Example;

struct _Example {
	const gchar *filename;
	const gchar *title;
};

static GPtrArray *examples;

GtkWidget *html;

static const gchar *welcome =
"Czech (&#268;e&#353;tina) &#268;au, Ahoj, Dobr&#253; den<BR>"
"French (Français) Bonjour, Salut<BR>"
"Korean (한글)   안녕하세요, 안녕하십니까<BR>"
"Russian (Русский) Здравствуйте!<BR>"
"Chinese (Simplified) <span lang=\"zh-cn\">元气	开发</span><BR>"
"Chinese (Traditional) <span lang=\"zh-tw\">元氣	開發</span><BR>"
"Japanese <span lang=\"ja\">元気	開発<BR></FONT>";

static void
url_requested (GtkHTML *unused,
               const gchar *url,
               GtkHTMLStream *stream,
               gpointer data)
{
	gint fd;
	gchar *filename;

	filename = g_strconcat ("tests/", url, NULL);
	fd = open (filename, O_RDONLY);

	if (fd != -1) {
#define MY_BUF_SIZE 32768
		gchar *buf;
		gsize size;

		buf = alloca (MY_BUF_SIZE);
		while ((size = read (fd, buf, MY_BUF_SIZE)) > 0) {
			gtk_html_stream_write (stream, buf, size);
		}
		gtk_html_stream_close (stream, size == -1 ? GTK_HTML_STREAM_ERROR : GTK_HTML_STREAM_OK);
		close (fd);
	} else
		gtk_html_stream_close (stream, GTK_HTML_STREAM_ERROR);

	g_free (filename);
}

static gchar *
encode_html (const gchar *txt)
{
	GString *str;

	str = g_string_new (NULL);

	do {
		gunichar uc;

		uc = g_utf8_get_char (txt);
		if (uc > 160) {
			g_string_append_printf (str, "&#%u;", uc);
		} else {
			g_string_append_c (str, uc);
		}
	} while ((txt = g_utf8_next_char (txt)) && *txt);

	return g_string_free (str, FALSE);
}

static void
example_changed_cb (GtkComboBox *combo_box,
                    gpointer data)
{
	gint i = gtk_combo_box_get_active (combo_box);
	Example *example = examples->pdata[i];

	if (example->filename) {
		GtkHTMLStream *stream;

		stream = gtk_html_begin (GTK_HTML (html));
		url_requested (GTK_HTML (html), example->filename, stream, NULL);
	} else {
		gtk_html_load_from_string (GTK_HTML (html), encode_html (welcome), -1);
	}
}

static void
quit_cb (GtkWidget *button)
{
	gtk_main_quit ();
}

static void
dump_cb (GtkWidget *widget,
         gpointer data)
{
	g_print ("Object Tree\n");
	g_print ("-----------\n");

	gtk_html_debug_dump_tree (GTK_HTML (html)->engine->clue, 0);
}

static void
dump_simple_cb (GtkWidget *widget,
                gpointer data)
{
	g_print ("Simple Object Tree\n");
	g_print ("-----------\n");

	gtk_html_debug_dump_tree_simple (GTK_HTML (html)->engine->clue, 0);
}

static void
draw_page_cb (GtkPrintOperation *operation,
              GtkPrintContext *context,
              gint page_nr,
              gpointer user_data)
{
	gtk_html_print_page (GTK_HTML (html), context);
}

static void
print_cb (GtkWidget *widget,
          gpointer data)
{
	GtkPrintOperation *operation;

	operation = gtk_print_operation_new ();
	gtk_print_operation_set_n_pages (operation, 1);

	g_signal_connect (
		operation, "draw-page",
		G_CALLBACK (draw_page_cb), NULL);

	gtk_print_operation_run (
		operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
		NULL, NULL);

	g_object_unref (operation);
}

/* We want to sort "a2" < "b1" < "B1" < "b2" < "b12". Vastly
 * overengineered
 */
static gint
compare_examples (gconstpointer a,
                  gconstpointer b)
{
	const Example *example_a = *(const Example *const *) a;
	const Example *example_b = *(const Example *const *) b;
	gchar *a_fold, *b_fold;
	const guchar *p, *q;
	gint result = 0;

	/* Special case "Welcome" to sort first */
	if (!example_a->filename)
		return -1;
	if (!example_b->filename)
		return 1;

	a_fold = g_utf8_casefold (example_a->title, -1);
	b_fold = g_utf8_casefold (example_b->title, -1);
	p = (const guchar *) a_fold;
	q = (const guchar *) b_fold;

	while (*p && *q) {
		gboolean p_digit = g_ascii_isdigit (*p);
		gboolean q_digit = g_ascii_isdigit (*q);

		if (p_digit && !q_digit) {
			result = 1;
			goto out;
		}
		else if (!p_digit && q_digit) {
			result = -1;
			goto out;
		}

		if (p_digit) {
			gint num_a = atoi ((const gchar *) p);
			gint num_b = atoi ((const gchar *) q);

			if (num_a < num_b) {
				result = -1;
				goto out;
			}
			else if (num_a > num_b) {
				result = 1;
				goto out;
			}

			while (g_ascii_isdigit (*p))
				p++;
			while (g_ascii_isdigit (*q))
				q++;

		} else {
			gint p_len = 1, q_len = 1;
			gchar *p_str, *q_str;

			while (*(p + p_len) && !g_ascii_isdigit (*(p + p_len)))
				p_len++;
			while (*(q + q_len) && !g_ascii_isdigit (*(q + q_len)))
				q_len++;

			p_str = g_strndup ((gchar *) p, p_len);
			q_str = g_strndup ((gchar *) q, q_len);

			result = g_utf8_collate (p_str, q_str);
			g_free (p_str);
			g_free (q_str);

			if (result != 0)
				goto out;

			p += p_len;
			q += p_len;
		}

		p++;
		q++;
	}

	if (*p)
		result = 1;
	else if (*q)
		result = -1;
	else
		result = g_utf8_collate (example_a->title, example_b->title);

 out:
	g_free (a_fold);
	g_free (b_fold);

	return result;
}

static void
find_examples (void)
{
	GDir *dir;
	GError *error;
	Example *example;

	examples = g_ptr_array_new ();

	example = g_new (Example, 1);
	example->filename = NULL;
	example->title = "Welcome";
	g_ptr_array_add (examples, example);

	dir = g_dir_open ("tests", 0, &error);
	if (!dir) {
		g_printerr ("Cannot open tests directory: %s\n", error->message);
		return;
	}

	while (TRUE) {
		const gchar *name = g_dir_read_name (dir);

		if (!name)
			break;
		if (!g_str_has_suffix (name, ".html"))
			continue;

		example = g_new (Example, 1);
		example->filename = g_strdup (name);
		example->title = g_strndup (name, strlen (name) - 5);

		g_ptr_array_add (examples, example);
		qsort (examples->pdata, examples->len, sizeof (gpointer), compare_examples);
	}

	g_dir_close (dir);
}

gint
main (gint argc,
      gchar **argv)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *combo_box;
	GtkWidget *swindow;
	GtkWidget *action_button;
	gint i = 0;

	gtk_init (&argc, &argv);

	find_examples ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	swindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	html = gtk_html_new_from_string (encode_html (welcome), -1);
	g_signal_connect (html, "url_requested", G_CALLBACK (url_requested), NULL);

	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_container_add (GTK_CONTAINER (swindow), html);
	gtk_box_pack_start (GTK_BOX (vbox), swindow, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	combo_box = gtk_combo_box_text_new ();
	for (i = 0; i < examples->len; i++) {
		Example *example = examples->pdata[i];
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_box), example->title);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
	g_signal_connect (combo_box, "changed", G_CALLBACK (example_changed_cb), NULL);

	gtk_box_pack_start (GTK_BOX (hbox), combo_box, FALSE, FALSE, 0);

	action_button = gtk_button_new_with_label ("Dump");
	gtk_box_pack_end (GTK_BOX (hbox), action_button, FALSE, FALSE, 0);
	g_signal_connect (action_button, "clicked", G_CALLBACK (dump_cb), NULL);
	action_button = gtk_button_new_with_label ("Dump simple");
	gtk_box_pack_end (GTK_BOX (hbox), action_button, FALSE, FALSE, 0);
	g_signal_connect (action_button, "clicked", G_CALLBACK (dump_simple_cb), NULL);
	action_button = gtk_button_new_with_label ("Print");
	gtk_box_pack_end (GTK_BOX (hbox), action_button, FALSE, FALSE, 0);
	g_signal_connect (action_button, "clicked", G_CALLBACK (print_cb), window);
	action_button = gtk_button_new_with_label ("Quit");
	gtk_box_pack_end (GTK_BOX (hbox), action_button, FALSE, FALSE, 0);
	g_signal_connect (action_button, "clicked", G_CALLBACK (quit_cb), NULL);

	gtk_window_set_title (GTK_WINDOW (window), _("GtkHTML Test"));
	gtk_window_set_default_size (GTK_WINDOW (window), 500, 500);

	gtk_widget_show_all (window);

	gtk_main ();

	return 0;

}
