/******************************************************************************
 * @file            lib.c
 *****************************************************************************/
#include    <ctype.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "ar.h"
#include    "lib.h"
#include    "report.h"
#include    "stdint.h"

static void print_help (void) {

    if (!program_name) {
        goto _exit;
    }
    
    fprintf (stderr, "Usage: %s command archive-file file...\n", program_name);
    fprintf (stderr, "Commands:\n\n");
    
    fprintf (stderr, "    d             delete file(s) from the archive\n");
    fprintf (stderr, "    q             quick append file(s) to the archive\n");
    fprintf (stderr, "    r             replace existing or insert new file(s) into the archive\n");
    fprintf (stderr, "    s             act as ranlib\n");
    fprintf (stderr, "    t             display contents of the archive\n");
    fprintf (stderr, "    x             extract file(s) from the archive\n");
    
    fprintf (stderr, "\n");
    
_exit:
    
    exit (EXIT_SUCCESS);

}

char *xstrdup (const char *str) {

    char *ptr = xmalloc (strlen (str) + 1);
    strcpy (ptr, str);
    
    return ptr;

}

void *xmalloc (unsigned long size) {

    void *ptr = malloc (size);
    
    if (ptr == NULL && size) {
    
        report_at (program_name, 0, REPORT_ERROR, "memory full (malloc)");
        exit (EXIT_FAILURE);
    
    }
    
    memset (ptr, 0, size);
    return ptr;

}

void *xrealloc (void *ptr, unsigned long size) {

    void *new_ptr = realloc (ptr, size);
    
    if (new_ptr == NULL && size) {
    
        report_at (program_name, 0, REPORT_ERROR, "memory full (realloc)");
        exit (EXIT_FAILURE);
    
    }
    
    return new_ptr;

}

void dynarray_add (void *ptab, long *nb_ptr, void *data) {

    int32_t nb, nb_alloc;
    void **pp;
    
    nb = *nb_ptr;
    pp = *(void ***) ptab;
    
    if ((nb & (nb - 1)) == 0) {
    
        if (!nb) {
            nb_alloc = 1;
        } else {
            nb_alloc = nb * 2;
        }
        
        pp = xrealloc (pp, nb_alloc * sizeof (void *));
        *(void ***) ptab = pp;
    
    }
    
    pp[nb++] = data;
    *nb_ptr = nb;

}

void parse_args (int *pargc, char ***pargv, int optind) {

    char **argv = *pargv;
    int argc = *pargc;
    
    const char *r;
    
    if (argc < 3) {
        print_help ();
    }
    
    r = argv[optind++];
    
check_options:
    
    if (r[0] == '-') {
        ++r;
    }
    
    while (*r != '\0') {
    
        char ch = *r++;
        
        if (ch == 'd') {
        
            state->del++;
            continue;
        
        }
        
        if (ch == 'm') {
        
            state->move++;
            continue;
        
        }
        
        if (ch == 'p') {
        
            state->print++;
            continue;
        
        }
        
        if (ch == 'q') {
        
            state->append++;
            continue;
        
        }
        
        if (ch == 'r') {
        
            state->replace++;
            continue;
        
        }
        
        if (ch == 's') {
        
            state->ranlib++;
            continue;
        
        }
        
        if (ch == 't') {
        
            state->display++;
            continue;
        
        }
        
        if (ch == 'x') {
        
            state->extract++;
            continue;
        
        }
        
        print_help ();
    
    }
    
    if (*(r = argv[optind++]) == '-') { goto check_options; }
    
    if (state->append + state->del + state->display + state->extract + state->move + state->print + state->ranlib + state->replace != 1) {
        print_help ();
    }
    
    state->outfile = xstrdup (r);
    
    while (optind < argc) {
        dynarray_add (&state->files, &state->nb_files, xstrdup (argv[optind++]));
    }

}
