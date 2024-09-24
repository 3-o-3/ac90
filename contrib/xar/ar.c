/******************************************************************************
 * @file            ar.c
 *****************************************************************************/
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "ar.h"
#include    "lib.h"
#include    "report.h"
#include    "stdint.h"

struct ar_state *state = 0;
const char *program_name = 0;

FILE *arfp = NULL;

int main (int argc, char **argv) {

    char ar_magic[8];
    long i;
    
    if (argc && *argv) {
    
        char *p;
        program_name = *argv;
        
        if ((p = strrchr (program_name, '/'))) {
            program_name = (p + 1);
        }
    
    }
    
    state = xmalloc (sizeof (*state));
    parse_args (&argc, &argv, 1);
    
    if (state->append || state->replace) {
    
        if ((arfp = fopen (state->outfile, "r+b")) == NULL) {
        
            if ((arfp = fopen (state->outfile, "w+b")) == NULL) {
            
                report_at (program_name, 0, REPORT_ERROR, "failed to create %s", state->outfile);
                return EXIT_FAILURE;
            
            }
            
            if (fwrite ("!<arch>\n", 8, 1, arfp) != 1) {
            
                fclose (arfp);
                
                report_at (program_name, 0, REPORT_ERROR, "failed whilst writing to %s", state->outfile);
                return EXIT_FAILURE;
            
            }
        
        }
        
        fclose (arfp);
    
    }
    
    if ((arfp = fopen (state->outfile, "r+b")) == NULL) {
    
        report_at (program_name, 0, REPORT_ERROR, "failed to open %s", state->outfile);
        return EXIT_FAILURE;
    
    }
    
    if (fread (ar_magic, 8, 1, arfp) != 1) {
    
        fclose (arfp);
        
        report_at (program_name, 0, REPORT_ERROR, "failed whilst reading %s", state->outfile);
        return EXIT_FAILURE;
    
    }
    
    if (memcmp ("!<arch>\n", ar_magic, 8)) {
    
        fclose (arfp);
        
        report_at (program_name, 0, REPORT_ERROR, "%s is not a valid archive file", state->outfile);
        return EXIT_FAILURE;
    
    }
    
    for (i = 0; i < state->nb_files; ++i) {
    
        if (state->append) {
        
            fseek (arfp, 0, SEEK_END);
            append (arfp, state->files[i]);
        
        } else if (state->replace) {
        
            fseek (arfp, 8, SEEK_SET);
            replace (state->files[i]);
        
        } else if (state->del) {
        
            fseek (arfp, 8, SEEK_SET);
            delete (state->files[i]);
        
        } else if (state->extract) {
        
            fseek (arfp, 8, SEEK_SET);
            extract (state->files[0]);
        
        }
    
    }
    
    if (state->display) {
    
        fseek (arfp, 8, SEEK_SET);
        display ();
    
    } else if (state->ranlib) {
    
        fseek (arfp, 8, SEEK_SET);
        ranlib ();
    
    }
    
    fclose (arfp);
    return EXIT_SUCCESS;

}
