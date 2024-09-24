/******************************************************************************
 * @file            lib.h
 *****************************************************************************/
#ifndef     _LIB_H
#define     _LIB_H

char *xstrdup (const char *str);

void *xmalloc (unsigned long size);
void *xrealloc (void *ptr, unsigned long size);

void dynarray_add (void *ptab, long *nb_ptr, void *data);
void parse_args (int *pargc, char ***pargv, int optind);

#endif      /* _LIB_H */
