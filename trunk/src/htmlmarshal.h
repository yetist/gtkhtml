
#ifndef __html_g_cclosure_marshal_MARSHAL_H__
#define __html_g_cclosure_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOL:OBJECT (htmlclosures.list:1) */
extern void html_g_cclosure_marshal_BOOLEAN__OBJECT (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);
#define html_g_cclosure_marshal_BOOL__OBJECT	html_g_cclosure_marshal_BOOLEAN__OBJECT

/* BOOL:ENUM (htmlclosures.list:2) */
extern void html_g_cclosure_marshal_BOOLEAN__ENUM (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);
#define html_g_cclosure_marshal_BOOL__ENUM	html_g_cclosure_marshal_BOOLEAN__ENUM

/* VOID:STRING,POINTER (htmlclosures.list:3) */
extern void html_g_cclosure_marshal_VOID__STRING_POINTER (GClosure     *closure,
                                                          GValue       *return_value,
                                                          guint         n_param_values,
                                                          const GValue *param_values,
                                                          gpointer      invocation_hint,
                                                          gpointer      marshal_data);

/* VOID:POINTER,INT (htmlclosures.list:4) */
extern void html_g_cclosure_marshal_VOID__POINTER_INT (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

/* VOID:STRING,STRING,STRING (htmlclosures.list:5) */
extern void html_g_cclosure_marshal_VOID__STRING_STRING_STRING (GClosure     *closure,
                                                                GValue       *return_value,
                                                                guint         n_param_values,
                                                                const GValue *param_values,
                                                                gpointer      invocation_hint,
                                                                gpointer      marshal_data);

/* VOID:INT,INT,FLOAT (htmlclosures.list:6) */
extern void html_g_cclosure_marshal_VOID__INT_INT_FLOAT (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:ENUM,ENUM,FLOAT (htmlclosures.list:7) */
extern void html_g_cclosure_marshal_VOID__ENUM_ENUM_FLOAT (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data);

/* VOID:INT,INT (htmlclosures.list:8) */
extern void html_g_cclosure_marshal_VOID__INT_INT (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:ENUM,ENUM (htmlclosures.list:9) */
extern void html_g_cclosure_marshal_VOID__ENUM_ENUM (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);

/* VOID:POINTER,BOOL,BOOL,BOOL (htmlclosures.list:10) */
extern void html_g_cclosure_marshal_VOID__POINTER_BOOLEAN_BOOLEAN_BOOLEAN (GClosure     *closure,
                                                                           GValue       *return_value,
                                                                           guint         n_param_values,
                                                                           const GValue *param_values,
                                                                           gpointer      invocation_hint,
                                                                           gpointer      marshal_data);
#define html_g_cclosure_marshal_VOID__POINTER_BOOL_BOOL_BOOL	html_g_cclosure_marshal_VOID__POINTER_BOOLEAN_BOOLEAN_BOOLEAN

/* POINTER:VOID (htmlclosures.list:11) */
extern void html_g_cclosure_marshal_POINTER__VOID (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:VOID (htmlclosures.list:12) */
#define html_g_cclosure_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

G_END_DECLS

#endif /* __html_g_cclosure_marshal_MARSHAL_H__ */

