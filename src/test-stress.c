#include <string.h>
#include <stdio.h>
#include <glib/gstring.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkwindow.h>
#include "gtkhtml.h"
#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluev.h"
#include "htmlcursor.h"
#include "htmlgdkpainter.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-text.h"
#include "htmlengine-save.h"
#include "htmlplainpainter.h"
#include "htmlselection.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmltext.h"

typedef struct {
	char *name;
	int (*test_function) (GtkHTML *html);
} Test;

static int test_level_1 (GtkHTML *html);

static Test tests[] = {
	{ "cursor movement", NULL },
	{ "level 1 - cut/copy/paste", test_level_1 },
	{ NULL, NULL }
};

static HTMLGdkPainter *gdk_painter = NULL;
static HTMLGdkPainter *plain_painter = NULL;

static void
set_format (GtkHTML *html, gboolean format_html)
{
	HTMLGdkPainter *p, *old_p;

	gtk_widget_ensure_style (GTK_WIDGET (html));
	
	if (!plain_painter) {
		gdk_painter = HTML_GDK_PAINTER (html->engine->painter);
		plain_painter = HTML_GDK_PAINTER (html_plain_painter_new (GTK_WIDGET (html), TRUE));

		g_object_ref (G_OBJECT (gdk_painter));
	}	
	

	if (format_html) {
		p = gdk_painter;
		old_p = plain_painter;
	} else {
		p = plain_painter;
		old_p = gdk_painter;
	}		

	if (html->engine->painter != (HTMLPainter *) p) {
		html_gdk_painter_unrealize (old_p);
		if (html->engine->window)
			html_gdk_painter_realize (p, html->engine->window);

		html_font_manager_set_default (&HTML_PAINTER (p)->font_manager,
					       HTML_PAINTER (old_p)->font_manager.variable.face,
					       HTML_PAINTER (old_p)->font_manager.fixed.face,
					       HTML_PAINTER (old_p)->font_manager.var_size,
					       HTML_PAINTER (old_p)->font_manager.var_points,
					       HTML_PAINTER (old_p)->font_manager.fix_size,
					       HTML_PAINTER (old_p)->font_manager.fix_points);

		html_engine_set_painter (html->engine, HTML_PAINTER (p));
		html_engine_schedule_redraw (html->engine);
	}
		
}

static int test_level_1 (GtkHTML *html)
{
	int i, total_len = 0;

	set_format (html, FALSE);

	srand (2);

	for (i = 0; i < 200; i ++) {
		int j, len = 1 + (int) (10.0*rand()/(RAND_MAX+1.0));
		char word [12];

		for (j = 0; j < len; j ++)
			word [j] = 'a' + (int) (26.0*rand()/(RAND_MAX+1.0));
		word [len] = ' ';
		word [len + 1] = 0;
		total_len += len + 1;

		html_engine_insert_text (html->engine, word, -1);
	}

	while (html_cursor_backward (html->engine->cursor, html->engine))
		;

	if (html->engine->cursor->position != 0 || html->engine->cursor->offset != 0)
		return FALSE;

	for (i = 0; i < 1000; i ++) {
		int j, new_pos, pos, len;

		len = 1 + (int) (120.0*rand()/(RAND_MAX+1.0));
		pos = MAX (0, (int) (((double) (total_len - len))*rand()/(RAND_MAX+1.0)));

		printf ("step: %d pos: %d len: %d\n", i, pos, len);

		switch ((int) (10.0*rand()/(RAND_MAX+1.0))) {
		case 0:
			/* cut'n'paste */
			printf ("cut'n'paste\n");
			html_cursor_jump_to_position (html->engine->cursor, html->engine, pos);
			html_engine_set_mark (html->engine);
			html_cursor_jump_to_position (html->engine->cursor, html->engine, pos + len);
			html_engine_cut (html->engine);

			new_pos = (int) (((double) (total_len - len))*rand()/(RAND_MAX+1.0));

			html_cursor_jump_to_position (html->engine->cursor, html->engine, new_pos);
			html_engine_paste (html->engine);
			break;
		case 1:
			/* insert text */
			printf ("insert text\n");
			html_cursor_jump_to_position (html->engine->cursor, html->engine, pos);
			for (j = 0; j < len; j ++) {
				int et = (int) (10.0*rand()/(RAND_MAX+1.0));
				if (et == 0)
					gtk_html_command (html, "insert-tab");
				else {
					char ch [2];

					if (et == 1)
						ch [0] = ' ';
				        else
						ch [0] = 'a' + (int) (26.0*rand()/(RAND_MAX+1.0));
					ch [1] = 0;
					html_engine_insert_text (html->engine, ch, 1);
				}
			}
			total_len += len;
			break;
		case 2:
			/* delete text */
			printf ("delete text\n");
			html_cursor_jump_to_position (html->engine->cursor, html->engine, pos + len);
			for (j = 0; j < len; j ++) {
				char ch [2];
				ch [0] = 'a' + (int) (26.0*rand()/(RAND_MAX+1.0));
				ch [1] = 0;
				gtk_html_command (html, "delete");
			}
			total_len -= len;
			break;
		case 3:
			/* undo */
			printf ("undo\n");
			for (j = 0; j < len; j ++) {
				html_engine_undo (html->engine);
			}
			break;
		case 4:
			/* redo */
			printf ("redo\n");
			for (j = 0; j < len; j ++) {
				html_engine_redo (html->engine);
			}
			break;
		case 5:
			/* cut'n'quoted paste */
			printf ("cut'n'quoted paste\n");
			html_cursor_jump_to_position (html->engine->cursor, html->engine, pos);
			html_engine_set_mark (html->engine);
			html_cursor_jump_to_position (html->engine->cursor, html->engine, pos + len);
			gtk_html_cut (html);

			new_pos = (int) (((double) (total_len - len))*rand()/(RAND_MAX+1.0));

			html_cursor_jump_to_position (html->engine->cursor, html->engine, new_pos);
			gtk_html_paste (html, TRUE);
			break;
		case 6:
			/* left */
			printf ("left\n");
			for (j = 0; j < 5*len; j ++) {
				html_cursor_left (html->engine->cursor, html->engine);
			}
			break;
		case 7:
			/* right */
			printf ("right\n");
			for (j = 0; j < 5*len; j ++) {
				html_cursor_right (html->engine->cursor, html->engine);
			}
			break;
		case 8:
			/* bol */
			printf ("beginning of line\n");
			html_cursor_beginning_of_line (html->engine->cursor, html->engine);
			break;
		case 9:
			/* eol */
			printf ("end of line\n");
			html_cursor_end_of_line (html->engine->cursor, html->engine);
			break;
		}

		html_engine_thaw_idle_flush (html->engine);

		while (html_cursor_backward (html->engine->cursor, html->engine))
			;
		if (html->engine->cursor->position != 0 || html->engine->cursor->offset != 0)
			return FALSE;
	}

	while (html_cursor_backward (html->engine->cursor, html->engine))
		;

	if (html->engine->cursor->position != 0 || html->engine->cursor->offset != 0)
		return FALSE;

	return TRUE;
}

int main (int argc, char *argv[])
{
	GtkWidget *win, *html_widget, *sw;
	GtkHTML *html;
	int i = 0, n_all, n_successful;

	if (argc > 1)
		return 0;

	gtk_init (&argc, &argv);

	html_widget = gtk_html_new ();
	html = GTK_HTML (html_widget);
	gtk_html_load_empty (html);
	gtk_html_set_editable (html, TRUE);
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_size_request (win, 600, 400);
	gtk_container_add (GTK_CONTAINER (sw), html_widget);
	gtk_container_add (GTK_CONTAINER (win), sw);

	gtk_widget_show_all (win);
	gtk_widget_show_now (win);

	n_all = n_successful = 0;

	fprintf (stderr, "\nGtkHTML test suite\n");
	fprintf (stderr, "--------------------------------------------------------------------------------\n");
	for (i = 0; tests [i].name; i ++) {
		int j, result;

		if (tests [i].test_function) {
			fprintf (stderr, "  %s ", tests [i].name);
			for (j = strlen (tests [i].name); j < 69; j ++)
				fputc ('.', stderr);
			result = (*tests [i].test_function) (html);
			fprintf (stderr, " %s\n", result ? "passed" : "failed");

			n_all ++;
			if (result)
				n_successful ++;
		} else {
			fprintf (stderr, "* %s\n", tests [i].name);
		}
	}

	fprintf (stderr, "--------------------------------------------------------------------------------\n");
	fprintf (stderr, "%d tests failed %d tests passed (of %d tests)\n\n", n_all - n_successful, n_successful, n_all);

	gtk_main ();

	return n_all != n_successful;
}
