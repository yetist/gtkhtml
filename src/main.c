#include <stdio.h>
#include <gnome.h>
#include "htmlengine.h"
#include "htmlstack.h"
#include "debug.h"

gchar string_to_parse[] = "<html><body bgcolor='white'><p align=center>Det här är tänkt att vara ett väldigt långt och blandat dokument som ska göra det enkelt att se alla typer av kombinationer <b>fetstil</b> <font size=5>Annorlunda storlekar</font> och <font color=blue>färger.</font></p><h1>Här kommer en hr</h1><hr><p align='center'>Lite text efter</p><p>Här ska jag skriva en jätttejättelång text för att äntligen börja på scrollningen och se om det fungerar, för det måste det ju naturligtvis göra om man ska börja lägga in så att den kan bli en WIDGET!! och det vill man ju att det ska vara så att man kan lägga in en fil som heter testgtkhtml.c i CVS sedan, för då ska den vara textbädd för gtkhtml-widgeten och sen ska jag även lägga in bättre stöd i HTMLObject, ta bort de dumma referenserna till Tex curr och head och tail. Men nu tror jag att jag har skrivit tillräckligt mycket så det så!</body></html>";

GtkWidget *window, *area, *box, *button;
HTMLEngine *engine;

gboolean cb_draw (GtkWidget *widget, gpointer data);
gboolean cb_expose (GtkWidget *widget, GdkEventExpose *event);
gboolean cb_configure (GtkWidget *widget, GdkEventConfigure *event);

gboolean
cb_draw (GtkWidget *widget, gpointer data)
{
	html_engine_draw ((HTMLEngine *)data);
	return FALSE;
}

gboolean
cb_expose (GtkWidget *widget, GdkEventExpose *event)
{
	html_engine_draw (engine);
	return FALSE;
}

gboolean
cb_configure (GtkWidget *widget, GdkEventConfigure *event)
{
	engine->width = event->width;
	engine->height = event->height;
	html_engine_calc_size (engine);

	return FALSE;
}

gint
main (gint argc, gchar *argv[])
{
	gtk_init (&argc, &argv);
	engine = html_engine_new ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), box);

	area = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (area), 400, 500);
	gtk_box_pack_start_defaults (GTK_BOX (box), area);
	
	/* FIXME: */
	engine->width = 400;
	engine->height = 500;

	gtk_widget_realize (area);
	engine->painter->area = area;
	engine->painter->gc = gdk_gc_new (area->window);
	gtk_signal_connect (GTK_OBJECT (area), "expose_event",
			    GTK_SIGNAL_FUNC (cb_expose), NULL);
	gtk_signal_connect (GTK_OBJECT (area), "configure_event",
			    GTK_SIGNAL_FUNC (cb_configure), NULL);

	/*
	while (fgets ((char *)&buffer, 32768, testfil)) {
		html_tokenizer_write (engine->ht, buffer);
	}
	*/
	html_engine_begin (engine);
	

	html_tokenizer_write (engine->ht, string_to_parse);

	html_engine_parse (engine);
	html_engine_parse_body (engine, engine->clue);

	gtk_widget_show_all (window);


	/*	g_print ("\nHTMLObject tree\n");
		g_print ("---------------\n");*/
	/*
	debug_dump_tree (engine->clue, 0);
	*/
	/*
	fclose (testfil);
	*/


	gtk_main ();

	return 0;
}










