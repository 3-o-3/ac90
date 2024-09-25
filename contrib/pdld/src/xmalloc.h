/******************************************************************************
 * @file            xmalloc.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stddef.h>

void *xmalloc (size_t size);
void *xcalloc (size_t num, size_t size);
void *xrealloc (void *p, size_t size);
char *xstrdup (const char *str);
size_t strnlen (const char *str, size_t max_len);
char *xstrndup (const char *str, size_t max_len);
int xstrcasecmp (const char *s1, const char *s2);
