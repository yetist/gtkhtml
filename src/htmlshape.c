/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
    
   Copyright (C) 2001, Ximian, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <glib.h>
#include <string.h>
#include <math.h>
#include "htmlshape.h"

struct _HTMLShape {
	HTMLShapeType type;
	gchar *url;
	gchar *target;
};

typedef struct _HTMLShapeRect HTMLShapeRect;
struct _HTMLShapeRect {
	HTMLShape shape;
	gint x0, y0;
	gint x1, y1;
};

typedef struct _HTMLShapeCircle HTMLShapeCircle;
struct _HTMLShapeCircle {
	HTMLShape shape;
	gint cx, cy;
	gint r;
};

typedef struct _HTMLShapePoly HTMLShapePoly;
struct _HTMLShapePoly {
	HTMLShape shape;
	gint count;
	gint *points;
};

#define HTML_DIST(x,y) ((gint)sqrt(x*x+y*y))

gboolean
html_shape_point (HTMLShape *shape, gint x, gint y)
{
	HTMLShapeRect *rect;
	HTMLShapeCircle *circle;

	switch (shape->type) {
	case HTML_SHAPE_RECT:
		rect = (HTMLShapeRect *)shape;
		
		if ((x >= rect->x0) 
		    && (x <= rect->x1) 
		    && (y >= rect->y0) 
		    && (y <= rect->y1))
			return TRUE;
		break;
	case HTML_SHAPE_CIRCLE:
		circle = (HTMLShapeCircle *)shape;
		
		if (HTML_DIST (x - circle->cx, y - circle->cy) <= circle->r)
			return TRUE;
		
		break;
	case HTML_SHAPE_POLY:
		break;
	}
	return FALSE;
}


static HTMLShape *
html_shape_construct (HTMLShape *shape, char *url, char *target, HTMLShapeType type) 
{
	shape->url = g_strdup (url);
	shape->target = g_strdup (target);
	shape->type = type;

	return shape;
}

static HTMLShape *
html_shape_rect_new (char *url, char *target, gint x0, gint y0, gint x1, gint y1)
{
	HTMLShapeRect *rect;
	
	rect = g_new (HTMLShapeRect, 1);
	rect->x0 = x0;
	rect->y0 = y0;
	rect->x1 = x1;
	rect->y1 = y1;

	return html_shape_construct ((HTMLShape *)rect, url, target, HTML_SHAPE_RECT);
}

static HTMLShape *
html_shape_circle_new (char *url, char *target, gint cx, gint cy, gint r)
{
	HTMLShapeCircle *circle;

	circle = g_new (HTMLShapeCircle, 1);
	circle->cx = cx;
	circle->cy = cy;
	circle->r = r;

	return html_shape_construct ((HTMLShape *)circle, url, target, HTML_SHAPE_CIRCLE);
}

static HTMLShape *
html_shape_poly_new (char *url, char *target, int count, int points[])
{
	HTMLShapePoly *poly;
	
	poly = g_new (HTMLShapePoly, 1);
	poly->count  = count;
	poly->points = points; 

	return html_shape_construct ((HTMLShape *)poly, url, target, HTML_SHAPE_POLY);
}

static HTMLShapeType
parse_shape_type (char *token) {
	HTMLShapeType type = HTML_SHAPE_DEFAULT;

	if (strncasecmp (token, "rect", 4) == 0)
		type = HTML_SHAPE_RECT;
	else if (strncasecmp (token, "poly", 4) == 0)
		type = HTML_SHAPE_POLY;
	else if (strncasecmp (token, "circle", 6) == 0)
		type = HTML_SHAPE_CIRCLE;

	return type;
}

char *
html_shape_get_url (HTMLShape *shape)
{
	return shape->url;
}

HTMLShape *
html_shape_new (char *type, char *coords, char *url, char *target)
{
	HTMLShape *shape = NULL;

	switch (parse_shape_type (type)) {
	case HTML_SHAPE_RECT:
	{
		int x0, y0, x1, y1;
		sscanf( coords, "%d,%d,%d,%d", &x0, &y0, &x1, &y1 );
		//gtk_html_debug_log (e->widget, "Area Rect %d, %d, %d, %d\n", x0, y0, x1, y1);
		shape = html_shape_rect_new (url, target, x0, y0, x1, y1);
		break;
	}
	case HTML_SHAPE_CIRCLE:
	{
		int cx, cy, r;
		sscanf (coords, "%d,%d,%d", &cx, &cy, &r);
		//gtk_html_debug_log (e->widget, "Area Circle %d, %d, %d\n", cx, cy, r);
		shape = html_shape_circle_new (url, target, cx, cy, r);
	break;
	}
#if 0				
	case HTML_SHAPE_POLY:
	{
		gtk_html_debug_log (e->widget, "Area Poly " );
		int count = 0, x, y;
		QPointArray parray;
		const char *ptr = coords;
		while ( ptr )
		{
			x = atoi( ptr );
			ptr = strchr( ptr, ',' );
			if ( ptr )
			{
				y = atoi( ++ptr );
				parray.resize( count + 1 );
				parray.setPoint( count, x, y );
				gtk_html_debug_log (e->widget, "%d, %d  ", x, y );
				count++;
				ptr = strchr( ptr, ',' );
				if ( ptr ) ptr++;
			}
		}
		gtk_html_debug_log (e->widget, "\n" );
		if ( count > 2 ) 
			//html_shape_poly_new 
			}
	break;
#endif
	default:
	}
	return shape;
}

void
html_shape_destroy (HTMLShape *shape)
{
	g_free (shape->url);
	g_free (shape->target);

	if (shape->type == HTML_SHAPE_POLY) {
		/* free poly stuff */
	}
}
		


