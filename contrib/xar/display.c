/******************************************************************************
 * @file            display.c
 *****************************************************************************/
#include    <stdio.h>
#include    <string.h>

#include    "ar.h"
#include    "report.h"

void display (void) {

    char temp[17] = { 0 };
    int i;
    
    for (;;) {
    
        struct ar_header hdr;
        long bytes;
        
        if (fread (&hdr, sizeof (hdr), 1, arfp) != 1) {
        
            if (feof (arfp)) {
                break;
            }
            
            report_at (program_name, 0, REPORT_ERROR, "failed whilst reading '%s'", state->outfile);
            return;
        
        }
        
        bytes = conv_dec (hdr.size, 10);
        
        if (bytes % 2) {
            bytes++;
        }
        
        fseek (arfp, bytes, SEEK_CUR);
        
        if (memcmp (hdr.name, "/", 1) == 0) {
            continue;
        }
        
        memcpy (temp, hdr.name, 16);
        
        for (i = 16; i >= 0; --i) {
        
            if (temp[i] == 0x20) {
                temp[i] = '\0';
            }
        
        }
        
        printf ("%s\n", temp);
    
    }

}
