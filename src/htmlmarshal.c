/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Author: Radek Doulik
    Copyright (C) 2002, Ximian Inc.

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

#include "htmlmarshal.h"

void
html_g_cclosure_marshal_VOID__STRING_POINTER (GClosure     *closure,
					      GValue       *return_value,
					      guint         n_param_values,
					      const GValue *param_values,
					      gpointer      invocation_hint,
					      gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__STRING_POINTER) (gpointer     data1,
							       const gchar *arg_1,
							       gpointer     arg_2,
							       gpointer     data2);

	register HTMLGMarshalFunc_VOID__STRING_POINTER ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__STRING_POINTER) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_string (param_values + 1),
	    g_value_get_pointer (param_values + 2),
	    data2);
}

void
html_g_cclosure_marshal_BOOL__POINTER (GClosure     *closure,
				       GValue       *return_value,
				       guint         n_param_values,
				       const GValue *param_values,
				       gpointer      invocation_hint,
				       gpointer      marshal_data)
{
	typedef gboolean (*HTMLGMarshalFunc_BOOL__POINTER) (gpointer     data1,
							    gpointer     arg_1,
							    gpointer     data2);

	register HTMLGMarshalFunc_BOOL__POINTER ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	gboolean v_return;

	g_return_if_fail (n_param_values == 2);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_BOOL__POINTER) (marshal_data ? marshal_data : cc->callback);

	v_return = ff (data1,
		       g_value_get_pointer (param_values + 1),
		       data2);

	g_value_set_boolean (return_value, v_return);
}

void
html_g_cclosure_marshal_VOID__POINTER_INT (GClosure     *closure,
					   GValue       *return_value,
					   guint         n_param_values,
					   const GValue *param_values,
					   gpointer      invocation_hint,
					   gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__POINTER_INT) (gpointer     data1,
							    gpointer     arg_1,
							    gint         arg_2,
							    gpointer     data2);

	register HTMLGMarshalFunc_VOID__POINTER_INT ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__POINTER_INT) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_pointer (param_values + 1),
	    g_value_get_int (param_values + 2),
	    data2);
}

void
html_g_cclosure_marshal_VOID__POINTER_POINTER_POINTER (GClosure     *closure,
						       GValue       *return_value,
						       guint         n_param_values,
						       const GValue *param_values,
						       gpointer      invocation_hint,
						       gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__POINTER_POINTER_POINTER) (gpointer     data1,
							       gpointer     arg_1,
							       gpointer     arg_2,
							       gpointer     arg_3,
							       gpointer     data2);

	register HTMLGMarshalFunc_VOID__POINTER_POINTER_POINTER ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 4);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__POINTER_POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_pointer (param_values + 1),
	    g_value_get_pointer (param_values + 2),
	    g_value_get_pointer (param_values + 3),
	    data2);
}

void
html_g_cclosure_marshal_VOID__POINTER_POINTER (GClosure     *closure,
					       GValue       *return_value,
					       guint         n_param_values,
					       const GValue *param_values,
					       gpointer      invocation_hint,
					       gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__POINTER_POINTER) (gpointer     data1,
							       gpointer     arg_1,
							       gpointer     arg_2,
							       gpointer     data2);

	register HTMLGMarshalFunc_VOID__POINTER_POINTER ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_pointer (param_values + 1),
	    g_value_get_pointer (param_values + 2),
	    data2);
}

void
html_g_cclosure_marshal_VOID__INT_INT_FLOAT (GClosure     *closure,
					     GValue       *return_value,
					     guint         n_param_values,
					     const GValue *param_values,
					     gpointer      invocation_hint,
					     gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__INT_INT_FLOAT) (gpointer     data1,
							      gint         arg_1,
							      gint         arg_2,
							      gfloat       arg_3,
							      gpointer     data2);

	register HTMLGMarshalFunc_VOID__INT_INT_FLOAT ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 4);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__INT_INT_FLOAT) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_int (param_values + 1),
	    g_value_get_int (param_values + 2),
	    g_value_get_float (param_values + 3),
	    data2);
}

void
html_g_cclosure_marshal_VOID__ENUM_ENUM_FLOAT (GClosure     *closure,
					       GValue       *return_value,
					       guint         n_param_values,
					       const GValue *param_values,
					       gpointer      invocation_hint,
					       gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__ENUM_ENUM_FLOAT) (gpointer     data1,
								gint         arg_1,
								gint         arg_2,
								gfloat       arg_3,
								gpointer     data2);

	register HTMLGMarshalFunc_VOID__ENUM_ENUM_FLOAT ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 4);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__ENUM_ENUM_FLOAT) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_enum (param_values + 1),
	    g_value_get_enum (param_values + 2),
	    g_value_get_float (param_values + 3),
	    data2);
}

void
html_g_cclosure_marshal_VOID__INT_INT (GClosure     *closure,
				       GValue       *return_value,
				       guint         n_param_values,
				       const GValue *param_values,
				       gpointer      invocation_hint,
				       gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__INT_INT) (gpointer     data1,
							gint         arg_1,
							gint         arg_2,
							gpointer     data2);

	register HTMLGMarshalFunc_VOID__INT_INT ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__INT_INT) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_int (param_values + 1),
	    g_value_get_int (param_values + 2),
	    data2);
}

void
html_g_cclosure_marshal_VOID__ENUM_ENUM (GClosure     *closure,
					 GValue       *return_value,
					 guint         n_param_values,
					 const GValue *param_values,
					 gpointer      invocation_hint,
					 gpointer      marshal_data)
{
	typedef void (*HTMLGMarshalFunc_VOID__ENUM_ENUM) (gpointer     data1,
							  gint         arg_1,
							  gint         arg_2,
							  gpointer     data2);

	register HTMLGMarshalFunc_VOID__ENUM_ENUM ff;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
		{
			data1 = closure->data;
			data2 = g_value_peek_pointer (param_values + 0);
		}
	else
		{
			data1 = g_value_peek_pointer (param_values + 0);
			data2 = closure->data;
		}
	ff = (HTMLGMarshalFunc_VOID__ENUM_ENUM) (marshal_data ? marshal_data : cc->callback);

	ff (data1,
	    g_value_get_enum (param_values + 1),
	    g_value_get_enum (param_values + 2),
	    data2);
}
