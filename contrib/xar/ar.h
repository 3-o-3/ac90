/******************************************************************************
 * @file            ar.h
 *****************************************************************************/
#ifndef     _AR_H
#define     _AR_H

struct ar_state {

    char **files;
    long nb_files;
    
    const char *outfile;
    
    int del, move, print;
    int append, replace, ranlib;
    int display, extract;

};

extern struct ar_state *state;
extern const char *program_name;

struct ar_header {

    char name[16];
    char mtime[12];
    char owner[6];
    char group[6];
    char mode[8];
    char size[10];
    char endsig[2];

};

#include    <stdio.h>

#include    "stdint.h"

extern FILE *arfp;
uint32_t conv_dec (char *str, int32_t max);

void append  (FILE *ofp, char *fname);
void delete  (char *fname);
void display (void);
void extract (char *fname);
void ranlib  (void);
void replace (char *fname);

#endif      /* _AR_H */
