/******************************************************************************
 * @file            ranlib.c
 *****************************************************************************/
#include    <limits.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "ar.h"
#include    "lib.h"
#include    "report.h"

#define     GET_INT32(arr)              ((int32_t) arr[0] | (((int32_t) arr[1]) << 8) | (((int32_t) arr[2]) << 16) | (((int32_t) arr[3]) << 24))

#define     GET_UINT16(arr)             ((uint32_t) arr[0] | (((uint32_t) arr[1]) << 8))
#define     GET_UINT32(arr)             ((uint32_t) arr[0] | (((uint32_t) arr[1]) << 8) | (((uint32_t) arr[2]) << 16) | (((uint32_t) arr[3]) << 24))

struct aout_exec {

    unsigned char a_info[4];
    unsigned char a_text[4];
    unsigned char a_data[4];
    unsigned char a_bss[4];
    unsigned char a_syms[4];
    unsigned char a_entry[4];
    unsigned char a_trsize[4];
    unsigned char a_drsize[4];

};

struct aout_nlist {

    unsigned char n_strx[4];
    unsigned char n_type;
    
    unsigned char n_other;
    unsigned char n_desc[2];
    
    unsigned char n_value[4];

};

struct coff_exec {

    unsigned char Machine[2];
    unsigned char NumberOfSections[2];
    
    unsigned char TimeDateStamp[4];
    unsigned char PointerToSymbolTable[4];
    unsigned char NumberOfSymbols[4];
    
    unsigned char SizeOfOptionalHeader[2];
    unsigned char Characteristics[2];

};

struct coff_symbol {

    char Name[8];
    unsigned char Value[4];
    
    unsigned char SectionNumber[2];
    unsigned char Type[2];
    
    unsigned char StorageClass[1];
    unsigned char NumberOfAuxSymbols[1];

};

struct strtab {

    const char *name;
    long length, offset;

};

struct gstrtab {

    long count, max;
    struct strtab *strtabs;

};

static struct gstrtab gstrtab = { 0, 64, NULL };

static int add_strtab (struct gstrtab *gstrtab, struct strtab *strtab) {

    if (gstrtab->strtabs == NULL) {
    
        if ((gstrtab->strtabs = malloc (gstrtab->max * sizeof (*strtab))) == NULL) {
            return 1;
        }
    
    }
    
    if (gstrtab->count >= gstrtab->max) {
    
        void *tmp;
        
        gstrtab->max *= 2;
        
        if ((tmp = realloc (gstrtab->strtabs, gstrtab->max * sizeof (*strtab))) == NULL) {
            return 1;
        }
        
        gstrtab->strtabs = tmp;
    
    }
    
    gstrtab->strtabs[gstrtab->count] = *strtab;
    gstrtab->count++;
    
    return 0;

}

static void aout_get_symbols (void *object, long offset) {

    struct aout_exec *hdr = (struct aout_exec *) object;
    
    long sym_start = sizeof (*hdr) + GET_UINT32 (hdr->a_text) + GET_UINT32 (hdr->a_data) + GET_UINT32 (hdr->a_trsize) + GET_UINT32 (hdr->a_drsize);
    long strtab_start = sym_start + GET_UINT32 (hdr->a_syms);
    
    while (sym_start < strtab_start) {
    
        struct aout_nlist nlist;
        memcpy (&nlist, (char *) object + sym_start, sizeof (nlist));
        
        if (nlist.n_type == 5 || nlist.n_type == 7 || nlist.n_type == 9) {
        
            struct strtab *strtab;
            char *symname = (char *) object + strtab_start + GET_INT32 (nlist.n_strx);
            
            strtab = xmalloc (sizeof (*strtab));
            strtab->length = strlen (symname);
            
            strtab->name = xstrdup (symname);
            strtab->offset = offset;
            
            add_strtab (&gstrtab, strtab);
        
        }
        
        sym_start += sizeof (nlist);
    
    }

}

static void coff_get_symbols (void *object, long offset) {

    struct coff_exec *hdr = (struct coff_exec *) object;
    
    long sym_start = GET_UINT32 (hdr->PointerToSymbolTable);
    long sym_cnt = GET_UINT32 (hdr->NumberOfSymbols);
    long string_table_start = sym_start + (sizeof (struct coff_symbol) * sym_cnt);
    
    while (sym_cnt--) {
    
        struct coff_symbol sym;
        memcpy (&sym, (char *) object + sym_start, sizeof (sym));
        
        if (sym.StorageClass[0] == 2 && GET_UINT16 (sym.SectionNumber) != 0) {
        
            struct strtab *strtab;
            
            if (sym.Name[0] != 0) {
            
                int i, len;
                
                for (i = 0, len = 0; i < 8; i++) {
                
                    if (sym.Name[i] == '\0') {
                        break;
                    }
                    
                    len++;
                
                }
                
                strtab = xmalloc (sizeof (*strtab));
                strtab->length = len;
                
                strtab->name = xstrdup (sym.Name);
                strtab->offset = offset;
                
                add_strtab (&gstrtab, strtab);
            
            } else {
            
                unsigned char offset1 = (unsigned char) sym.Name[4];
                unsigned char offset2 = (unsigned char) sym.Name[5];
                unsigned char offset3 = (unsigned char) sym.Name[6];
                unsigned char offset4 = (unsigned char) sym.Name[7];
                
                long final_offset = ((uint32_t) offset1 | (((uint32_t) offset2) << 8) | (((uint32_t) offset3) << 16) | (((uint32_t) offset4) << 24));
                final_offset += string_table_start;
                
                strtab = xmalloc (sizeof (*strtab));
                strtab->length = strlen ((char *) object + final_offset);
                
                strtab->name = xstrdup ((char *) object + final_offset);
                strtab->offset = offset;
                
                add_strtab (&gstrtab, strtab);
            
            }
        
        }
        
        sym_start += sizeof (sym);
    
    }

}

void ranlib (void) {

    FILE *tfp = tmpfile ();
    long offset = 0;
    
    struct ar_header header;
    long bytes, i, j, len, read, val;
    
    unsigned char *object;
    void *contents;
    
    char temp[16];
    int valid = 0;
    
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
        
        if (memcmp (hdr.name, "/", 1) == 0) {
        
            fseek (arfp, bytes, SEEK_CUR);
            continue;
        
        }
        
        object = xmalloc (bytes);
        
        if (fread (object, bytes, 1, arfp) != 1) {
        
            free (object);
            
            report_at (program_name, 0, REPORT_ERROR, "failed to read %ld bytes from %s", bytes, state->outfile);
            exit (EXIT_FAILURE);
            
        
        }
        
        valid = ((object[0] == 0x07 && object[1] == 0x01) || (object[0] == 0x4C && object[1] == 0x01) || (object[0] == 0x64 && object[1] == 0x86));
        
        if (!valid) {
        
            free (object);
            
            offset += sizeof (hdr);
            offset += bytes;
            
            fseek (arfp, bytes, SEEK_CUR);
            continue;
        
        }
        
        if (object[0] == 0x07 && object[1] == 0x01) {
            aout_get_symbols (object, offset + 8);
        } else {
            coff_get_symbols (object, offset + 8);
        }
        
        free (object);
        
        offset += sizeof (hdr);
        offset += bytes;
    
    }
    
    fseek (arfp, 8, SEEK_SET);
    bytes = 0;
    
    for (i = 0; i < gstrtab.count; ++i) {
        bytes += gstrtab.strtabs[i].length + 5;
    }
    
    for (i = 0; i < gstrtab.count; ++i) {
    
        gstrtab.strtabs[i].offset += bytes;
        
        if (bytes % 2) {
            gstrtab.strtabs[i].offset++;
        }
    
    }
    
    if (fwrite ("!<arch>\x0A", 8, 1, tfp) != 1) {
    
        fclose (tfp);
        
        report_at (program_name, 0, REPORT_ERROR, "failed whist writing ar header");
        return;
    
    }
    
    memset (temp, 0x20, 16);
    temp[0] = '0';
    
    len = 1;
    memcpy (header.name, "/", len);
    
    while (len < 16) {
        header.name[len++] = 0x20;
    }
    
    memcpy (header.mtime, temp, 12);
    memcpy (header.owner, temp, 6);
    memcpy (header.group, temp, 6);
    memcpy (header.mode, temp, 8);
    
    len = sprintf (temp, "%ld", bytes + 4);
    temp[len] = 0x20;
    
    memcpy (header.size, temp, 10);
    
    header.endsig[0] = 0x60;
    header.endsig[1] = 0x0A;
    
    if (fwrite (&header, sizeof (header), 1, tfp) != 1) {
    
        fclose (tfp);
        
        report_at (program_name, 0, REPORT_ERROR, "failed whilst writing header");
        return;
    
    }
    
    val = gstrtab.count;
    
    for (i = 0; i < 4; ++i) {
        temp[4 - 1 - i] = (val >> (CHAR_BIT * i)) & UCHAR_MAX;
    }
    
    if (fwrite (temp, 4, 1, tfp) != 1) {
        exit (EXIT_FAILURE);
    }
    
    for (i = 0; i < gstrtab.count; ++i) {
    
        val = gstrtab.strtabs[i].offset + 4 + sizeof (header);
        
        for (j = 0; j < 4; ++j) {
            temp[4 - 1 - j] = (val >> (CHAR_BIT * j)) & UCHAR_MAX;
        }
        
        if (fwrite (temp, 4, 1, tfp) != 1) {
            exit (EXIT_FAILURE);
        }
    
    }
    
    for (i = 0; i < gstrtab.count; ++i) {
    
        const char *name = gstrtab.strtabs[i].name;
        long length = gstrtab.strtabs[i].length;
        
        if (fwrite (name, length, 1, tfp) != 1) {
            exit (EXIT_FAILURE);
        }
        
        temp[0] = '\0';
        
        if (fwrite (temp, 1, 1, tfp) != 1) {
            exit (EXIT_FAILURE);
        }
    
    }
    
    if (bytes % 2) {
    
        temp[0] = '\0';
        
        if (fwrite (temp, 1, 1, tfp) != 1) {
            exit (EXIT_FAILURE);
        }
    
    }
    
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
        
        if (bytes % 2) {
            bytes++;
        }
        
        if (memcmp (hdr.name, "/", 1) == 0) {
        
            fseek (arfp, bytes, SEEK_CUR);
            continue;
        
        }
        
        if (fwrite (&hdr, sizeof (hdr), 1, tfp) != 1) {
        
            fclose (tfp);
            
            report_at (program_name, 0, REPORT_ERROR, "failed whilst writing header");
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
                fclose (tfp);
                
                report_at (NULL, 0, REPORT_ERROR, "failed to read %ld bytes from %s", bytes, state->outfile);
                exit (EXIT_FAILURE);
            
            }
            
            bytes -= read;
            
            if (fwrite (contents, read, 1, tfp) != 1) {
            
                free (contents);
                fclose (tfp);
                
                report_at (NULL, 0, REPORT_ERROR, "failed to write temp file");
                exit (EXIT_FAILURE);
            
            }
        
        }
        
        free (contents);
    
    }
    
    fclose (arfp);
    remove (state->outfile);
    
    if ((arfp = fopen (state->outfile, "w+b")) == NULL) {
    
        fclose (tfp);
        
        report_at (program_name, 0, REPORT_ERROR, "failed to open %s for writing", state->outfile);
        exit (EXIT_FAILURE);
    
    }
    
    contents = xmalloc (512);
    bytes = ftell (tfp);
    
    fseek (arfp, 0, SEEK_SET);
    fseek (tfp, 0, SEEK_SET);
    
    for (;;) {
    
        if (bytes == 0 || feof (tfp)) {
            break;
        } else if (bytes >= 512) {
            read = 512;
        } else {
            read = bytes;
        }
        
        if (fread (contents, read, 1, tfp) != 1) {
            
            free (contents);
            fclose (tfp);
            
            report_at (program_name, 0, REPORT_ERROR, "failed whilst reading temp file");
            return;
        
        }
        
        bytes -= read;
        
        if (fwrite (contents, read, 1, arfp) != 1) {
        
            free (contents);
            fclose (tfp);
            
            report_at (program_name, 0, REPORT_ERROR, "failed whist writing %s", state->outfile);
            exit (EXIT_FAILURE);
        
        }
    
    }
    
    free (contents);
    fclose (tfp);

}
