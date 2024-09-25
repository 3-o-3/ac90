/******************************************************************************
 * @file            extract.c
 *****************************************************************************/
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "ar.h"
#include    "lib.h"
#include    "report.h"

void extract (char *fname) {

    FILE *ofp;
    
    char temp[17], *name, *p, *contents;
    long bytes, len, read;
    
    name = fname;
    
    if ((p = strrchr (fname, '/'))) {
        name = (p + 1);
    }
    
    len = strlen (name);
    
    if (len > 16) {
        len = 16;
    }
    
    memcpy (temp, name, len);
    temp[len] = '\0';
    
    for (;;) {
    
        struct ar_header hdr;
        
        if (fread (&hdr, sizeof (hdr), 1, arfp) != 1) {
        
            if (feof (arfp)) {
                break;
            }
            
            report_at (program_name, 0, REPORT_ERROR, "failed whilst reading '%s'", state->outfile);
            return;
        
        }
        
        bytes = conv_dec (hdr.size, 10);
        
        if (memcmp (hdr.name, "/", 1) == 0) {
        
            if (bytes % 2) {
                bytes++;
            }
            
            fseek (arfp, bytes, SEEK_CUR);
            continue;
        
        }
        
        if (memcmp (hdr.name, temp, len) != 0) {
        
            if (bytes % 2) {
                bytes++;
            }
            
            fseek (arfp, bytes, SEEK_CUR);
            continue;
        
        }
        
        if ((ofp = fopen (temp, "w+b")) == NULL) {
        
            report_at (program_name, 0, REPORT_ERROR, "failed to open %s for writing", temp);
            exit (EXIT_FAILURE);
        
        }
        
        contents = xmalloc (512);
        
        for (;;) {
        
            if (bytes == 0 || feof (arfp)) {
                break;
            } else if (bytes >= 512) {
                read = 512;
            } else {
                read = bytes;
            }
            
            if (fread (contents, read, 1, arfp) != 1) {
            
                free (contents);
                fclose (ofp);
                
                report_at (NULL, 0, REPORT_ERROR, "failed to read %ld bytes from %s", bytes, state->outfile);
                exit (EXIT_FAILURE);
            
            }
            
            bytes -= read;
            
            if (fwrite (contents, read, 1, ofp) != 1) {
            
                free (contents);
                fclose (ofp);
                
                report_at (NULL, 0, REPORT_ERROR, "failed to write %s file", temp);
                exit (EXIT_FAILURE);
            
            }
        
        }
        
        free (contents);
    
    }

}
