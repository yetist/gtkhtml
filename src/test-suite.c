#include <string.h>
#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include "gtkhtml.h"
#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluev.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-text.h"
#include "htmltext.h"

typedef struct {
	char *name;
	int (*test_function) (GtkHTML *html);
} Test;

static int test_cursor_beol (GtkHTML *html);
static int test_cursor_beol_rtl (GtkHTML *html);
static int test_cursor_left_right_on_items_boundaries (GtkHTML *html);
static int test_cursor_left_right_on_lines_boundaries (GtkHTML *html);
static int test_cursor_left_right_on_lines_boundaries_rtl (GtkHTML *html);
static int test_cursor_around_containers (GtkHTML *html);

static int test_quotes_in_div_block (GtkHTML *html);
static int test_capitalize_upcase_lowcase_word (GtkHTML *html);

static Test tests[] = {
	{ "cursor movement", NULL },
	{ "left/right on items boundaries", test_cursor_left_right_on_items_boundaries },
	{ "left/right on lines boundaries", test_cursor_left_right_on_lines_boundaries },
	{ "left/right on lines boundaries (RTL)", test_cursor_left_right_on_lines_boundaries_rtl },
	{ "begin/end of line", test_cursor_beol },
	{ "begin/end of line (RTL)", test_cursor_beol_rtl },
	{ "around containers", test_cursor_around_containers },
	{ "various fixed bugs", NULL },
	{ "outer quotes inside div block", test_quotes_in_div_block },
	{ "capitalize, upcase/lowcase word", test_capitalize_upcase_lowcase_word },
	{ NULL, NULL }
};

static void load_editable (GtkHTML *html, char *s)
{
	gtk_html_set_editable (html, FALSE);
	gtk_html_load_from_string (html, s, -1);
/* 	gtk_html_debug_dump_tree_simple (html->engine->clue, 0); */
	gtk_html_set_editable (html, TRUE);
}

static int test_cursor_around_containers (GtkHTML *html)
{
	load_editable (html, "abc<table><tr><td>abc</td></tr></table>");

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 9);
	if (html->engine->cursor->offset != 1
	    || html->engine->cursor->position != 9
	    || !html_cursor_beginning_of_line (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 4)
		return FALSE;

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 4);
	if (html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 4
	    || !html_cursor_end_of_line (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 1
	    || html->engine->cursor->position != 9)
		return FALSE;

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 5);
	if (html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 5
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 4)
		return FALSE;

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 8);
	if (html->engine->cursor->offset != 3
	    || html->engine->cursor->position != 8
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 1
	    || html->engine->cursor->position != 9)
		return FALSE;

	return TRUE;
}

static int test_cursor_left_right_on_items_boundaries (GtkHTML *html)
{
	load_editable (html, "ab<b>cde</b>ef");

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 1);
	if (html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 1
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 2
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 3)
		return FALSE;

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 3);
	if (html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 3
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 2
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 1)
		return FALSE;


	html_cursor_jump_to_position (html->engine->cursor, html->engine, 4);
	if (html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 4
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 5
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 6)
		return FALSE;

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 6);
	if (html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 6
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 5
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->position != html->engine->cursor->offset
	    || html->engine->cursor->position != 4)
		return FALSE;

	printf ("test_cursor_left_right_on_items_boundaries: passed\n");

	return TRUE;
}

static int test_cursor_left_right_on_lines_boundaries (GtkHTML *html)
{
	load_editable (html, "<pre>first line\nsecond line\nthird line");

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 11);
	if (html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 11
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 10
	    || html->engine->cursor->position != 10
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 11)
		return FALSE;

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 22);
	if (html->engine->cursor->offset != 11
	    || html->engine->cursor->position != 22
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 23
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 11
	    || html->engine->cursor->position != 22)
		return FALSE;

	printf ("test_cursor_left_right_on_lines_boundaries: passed\n");

	return TRUE;
}

static int test_cursor_left_right_on_lines_boundaries_rtl (GtkHTML *html)
{
	load_editable (html, "<pre>أوروبا, برمجيات الحاسوب + انترنيت :\nتصبح عالميا مع يونيكود\nأوروبا, برمجيات الحاسوب + انترنيت :");

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 36);
	if (html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 36
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 35
	    || html->engine->cursor->position != 35
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 36)
		return FALSE;

	html_cursor_jump_to_position (html->engine->cursor, html->engine, 58);
	if (html->engine->cursor->offset != 22
	    || html->engine->cursor->position != 58
	    || !html_cursor_left (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 59
	    || !html_cursor_right (html->engine->cursor, html->engine)
	    || html->engine->cursor->offset != 22
	    || html->engine->cursor->position != 58)
		return FALSE;

	printf ("test_cursor_left_right_on_lines_boundaries_rtl: passed\n");

	return TRUE;
}

static int test_cursor_beol (GtkHTML *html)
{
	load_editable (html, "<pre>simple line\nsecond line\n");

	/* first test it on 1st line */
	printf ("test_cursor_beol: testing 1st line eol\n");
	if (!html_engine_end_of_line (html->engine)
	    || html->engine->cursor->offset != html->engine->cursor->position
	    || html->engine->cursor->offset != 11)
		return FALSE;

	printf ("test_cursor_beol: testing 1st line bol\n");
	if (!html_engine_beginning_of_line (html->engine)
	    || html->engine->cursor->offset != html->engine->cursor->position
	    || html->engine->cursor->offset != 0)
		return FALSE;

	/* move to 2nd and test again */
	printf ("test_cursor_beol: testing 2nd line\n");
	if (!html_engine_move_cursor (html->engine, HTML_ENGINE_CURSOR_DOWN, 1))
		return FALSE;

	printf ("test_cursor_beol: testing 2nd line eol\n");
	if (!html_engine_end_of_line (html->engine)
	    || html->engine->cursor->offset != 11
	    || html->engine->cursor->position != 23)
		return FALSE;

	printf ("test_cursor_beol: testing 2nd line bol\n");
	if (!html_engine_beginning_of_line (html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 12)
		return FALSE;

	printf ("test_cursor_beol: passed\n");

	return TRUE;
}

static int test_cursor_beol_rtl (GtkHTML *html)
{
	load_editable (html, "<pre>أوروبا, برمجيات الحاسوب + انترنيت :\nتصبح عالميا مع يونيكود\n");

	/* first test it on 1st line */
	printf ("test_cursor_beol_rtl: testing 1st line eol\n");
	if (!html_engine_end_of_line (html->engine)
	    || html->engine->cursor->offset != html->engine->cursor->position
	    || html->engine->cursor->offset != 35)
		return FALSE;

	printf ("test_cursor_beol_rtl: testing 1st line bol\n");
	if (!html_engine_beginning_of_line (html->engine)
	    || html->engine->cursor->offset != html->engine->cursor->position
	    || html->engine->cursor->offset != 0)
		return FALSE;

	/* move to 2nd and test again */
	printf ("test_cursor_beol_rtl: testing 2nd line\n");
	if (!html_engine_move_cursor (html->engine, HTML_ENGINE_CURSOR_DOWN, 1))
		return FALSE;

	printf ("test_cursor_beol_rtl: testing 2nd line eol\n");
	if (!html_engine_end_of_line (html->engine)
	    || html->engine->cursor->offset != 22
	    || html->engine->cursor->position != 58)
		return FALSE;

	printf ("test_cursor_beol_rtl: testing 2nd line bol\n");
	if (!html_engine_beginning_of_line (html->engine)
	    || html->engine->cursor->offset != 0
	    || html->engine->cursor->position != 36)
		return FALSE;

	printf ("test_cursor_beol_rtl: passed\n");

	return TRUE;
}

static int
test_quotes_in_div_block (GtkHTML *html)
{
	GByteArray *flow_levels;
	HTMLEngine *e = html->engine;

	load_editable (html,
		       "<blockquote type=cite>"
		       "<div style=\"border: solid lightgray 5px;\">"
		       "quoted text with border"
		       "</div>"
		       "quoted text"
		       "</blockquote>");

	/* test for right object hierarhy */
	if (!e->clue || !HTML_CLUE (e->clue)->head || !HTML_IS_CLUEV (HTML_CLUE (e->clue)->head)
	    || !HTML_CLUE (HTML_CLUE (e->clue)->head)->head || !HTML_IS_CLUEFLOW (HTML_CLUE (HTML_CLUE (e->clue)->head)->head))
		return FALSE;

	flow_levels = HTML_CLUEFLOW (HTML_CLUE (HTML_CLUE (e->clue)->head)->head)->levels;

	/* test if levels are OK */
	if (!flow_levels || !flow_levels->len == 1 || flow_levels->data [0] != HTML_LIST_TYPE_BLOCKQUOTE_CITE)
		return FALSE;

	return TRUE;
}

static int
test_capitalize_upcase_lowcase_word (GtkHTML *html)
{
	HTMLObject *text_object;

	load_editable (html, "<pre>příliš běloučký kůň skákal přes pole sedmikrásek");

	html_engine_capitalize_word (html->engine);
	html_engine_capitalize_word (html->engine);
	html_engine_capitalize_word (html->engine);
	html_engine_capitalize_word (html->engine);
	html_engine_capitalize_word (html->engine);
	html_engine_capitalize_word (html->engine);
	html_engine_capitalize_word (html->engine);
	html_engine_thaw_idle_flush (html->engine);

	text_object = html_object_get_head_leaf (html->engine->clue);
	if (!text_object || !HTML_IS_TEXT (text_object) || strcmp (HTML_TEXT (text_object)->text, "Příliš Běloučký Kůň Skákal Přes Pole Sedmikrásek"))
		return FALSE;

	printf ("test_capitalize_upcase_lowcase_word: capitalize OK\n");

	if (!html_engine_beginning_of_line (html->engine))
		return FALSE;

	html_engine_upcase_downcase_word (html->engine, TRUE);
	html_engine_upcase_downcase_word (html->engine, TRUE);
	html_engine_upcase_downcase_word (html->engine, TRUE);
	html_engine_upcase_downcase_word (html->engine, TRUE);
	html_engine_upcase_downcase_word (html->engine, TRUE);
	html_engine_upcase_downcase_word (html->engine, TRUE);
	html_engine_upcase_downcase_word (html->engine, TRUE);
	html_engine_thaw_idle_flush (html->engine);

	text_object = html_object_get_head_leaf (html->engine->clue);
	if (!text_object || !HTML_IS_TEXT (text_object) || strcmp (HTML_TEXT (text_object)->text, "PŘÍLIŠ BĚLOUČKÝ KŮŇ SKÁKAL PŘES POLE SEDMIKRÁSEK"))
		return FALSE;

	printf ("test_capitalize_upcase_lowcase_word: upper OK\n");

	if (!html_engine_beginning_of_line (html->engine))
		return FALSE;

	html_engine_upcase_downcase_word (html->engine, FALSE);
	html_engine_upcase_downcase_word (html->engine, FALSE);
	html_engine_upcase_downcase_word (html->engine, FALSE);
	html_engine_upcase_downcase_word (html->engine, FALSE);
	html_engine_upcase_downcase_word (html->engine, FALSE);
	html_engine_upcase_downcase_word (html->engine, FALSE);
	html_engine_upcase_downcase_word (html->engine, FALSE);
	html_engine_thaw_idle_flush (html->engine);

	text_object = html_object_get_head_leaf (html->engine->clue);
	if (!text_object || !HTML_IS_TEXT (text_object) || strcmp (HTML_TEXT (text_object)->text, "příliš běloučký kůň skákal přes pole sedmikrásek"))
		return FALSE;

	printf ("test_capitalize_upcase_lowcase_word: lower OK\n");

	printf ("test_capitalize_upcase_lowcase_word: passed\n");

	return TRUE;
}

int main (int argc, char *argv[])
{
	GtkWidget *win, *html_widget;
	GtkHTML *html;
	int i = 0, n_all, n_successful;

	gtk_init (&argc, &argv);

	html_widget = gtk_html_new ();
	html = GTK_HTML (html_widget);
	gtk_html_load_empty (html);
	gtk_html_set_editable (html, TRUE);
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_add (GTK_CONTAINER (win), html_widget);

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

	return n_all != n_successful;
}
