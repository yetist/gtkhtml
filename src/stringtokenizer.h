#ifndef _STRINGTOKENIZER_H_
#define _STRINGTOKENIZER_H_

#include <glib.h>

struct _StringTokenizer {
	gchar *pos;
	gchar *end;
	gchar *buffer;
	gint bufLen;
};

typedef struct _StringTokenizer StringTokenizer;

StringTokenizer *string_tokenizer_new (void);
void             string_tokenizer_tokenize (StringTokenizer *t, const gchar *str, gchar *separators);
gboolean         string_tokenizer_has_more_tokens (StringTokenizer *t);
gchar           *string_tokenizer_next_token (StringTokenizer *t);

#endif /* _STRINGTOKENIZER_H_ */
