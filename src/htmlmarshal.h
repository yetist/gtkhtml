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

#include <glib-object.h>

void
html_g_cclosure_marshal_VOID__STRING_POINTER (GClosure     *closure,
					      GValue       *return_value,
					      guint         n_param_values,
					      const GValue *param_values,
					      gpointer      invocation_hint,
					      gpointer      marshal_data);
void
html_g_cclosure_marshal_BOOL__POINTER (GClosure     *closure,
				       GValue       *return_value,
				       guint         n_param_values,
				       const GValue *param_values,
				       gpointer      invocation_hint,
				       gpointer      marshal_data);
void
html_g_cclosure_marshal_VOID__POINTER_INT (GClosure     *closure,
					   GValue       *return_value,
					   guint         n_param_values,
					   const GValue *param_values,
					   gpointer      invocation_hint,
					   gpointer      marshal_data);
void
html_g_cclosure_marshal_VOID__POINTER_POINTER_POINTER (GClosure     *closure,
						       GValue       *return_value,
						       guint         n_param_values,
						       const GValue *param_values,
						       gpointer      invocation_hint,
						       gpointer      marshal_data);

void
html_g_cclosure_marshal_VOID__POINTER_POINTER (GClosure     *closure,
					       GValue       *return_value,
					       guint         n_param_values,
					       const GValue *param_values,
					       gpointer      invocation_hint,
					       gpointer      marshal_data);

void
html_g_cclosure_marshal_VOID__INT_INT_FLOAT (GClosure     *closure,
					     GValue       *return_value,
					     guint         n_param_values,
					     const GValue *param_values,
					     gpointer      invocation_hint,
					     gpointer      marshal_data);

void
html_g_cclosure_marshal_VOID__ENUM_ENUM_FLOAT (GClosure     *closure,
					       GValue       *return_value,
					       guint         n_param_values,
					       const GValue *param_values,
					       gpointer      invocation_hint,
					       gpointer      marshal_data);
void
html_g_cclosure_marshal_VOID__INT_INT (GClosure     *closure,
				       GValue       *return_value,
				       guint         n_param_values,
				       const GValue *param_values,
				       gpointer      invocation_hint,
				       gpointer      marshal_data);

void
html_g_cclosure_marshal_VOID__ENUM_ENUM (GClosure     *closure,
					 GValue       *return_value,
					 guint         n_param_values,
					 const GValue *param_values,
					 gpointer      invocation_hint,
					 gpointer      marshal_data);
