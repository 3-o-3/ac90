/******************************************************************************
 * @file            process.c
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "as.h"
#include "hashtab.h"
#include "cfi.h"

static const char *filename;
static unsigned long line_number;

static struct hashtab *pseudo_op_hashtab;

char lex_table[256] = {

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,                             /* SPACE!"#$%&'()*+,-./ */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,                             /* 0123456789:;<=>?     */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,                             /* @ABCDEFGHIJKLMNO     */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 3,                             /* #PQRSTUVWXYZ[\]^_    */
    0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,                             /* 'abcdefghijklmno     */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0                              /* pqrstuvwxyz{|}~      */

};

/* Index: a character
 * Output:
 *  1 - the character ends a line
 *  2 - the character is a line separator
 *
 * Line separators allowing having multiple logical lines on a single physical line.
 */
char is_end_of_line[256] = {

    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 /* '\0' and '\n' */

};

static char is_comment_at_the_start_of_line[256] = {0}; /* Fully target dependent. */

static void do_org (section_t section, struct expr *expr, unsigned long fill_value);

static section_t get_known_section_expression (char **pp, struct expr *expr) {

    section_t section = machine_dependent_simplified_expression_read_into (pp, expr);
    
    if (expr->type == EXPR_TYPE_INVALID || expr->type == EXPR_TYPE_ABSENT) {
    
        as_error ("expected address expression");
        
        expr->type = EXPR_TYPE_CONSTANT;
        expr->add_number = 0;
        
        section = absolute_section;
    
    }
    
    if (section == undefined_section) {
    
        if (expr->add_symbol && symbol_get_section (expr->add_symbol) != expr_section) {
            as_warn ("symbol \"%s\" undefined; zero assumed", symbol_get_name (expr->add_symbol));
        } else {
            as_warn ("some symbol undefined; zero assumed");
        }
        
        expr->type = EXPR_TYPE_CONSTANT;
        expr->add_number = 0;
        
        section = absolute_section;
    
    }
    
    return section;

}

static void handler_constant (char **pp, int size, int is_rva)
{
    struct expr expr;
    
    do {
        machine_dependent_simplified_expression_read_into (pp, &expr);
        
        if (is_rva) {
            if (expr.type == EXPR_TYPE_SYMBOL) {
                expr.type = EXPR_TYPE_SYMBOL_RVA;
            } else {
                as_error ("rva without symbol.");
            }
        }
        
        if (expr.type == EXPR_TYPE_CONSTANT) {
            machine_dependent_number_to_chars (frag_increase_fixed_size (size), expr.add_number, size);
        } else if (expr.type != EXPR_TYPE_INVALID) {
            fixup_new_expr (current_frag, current_frag->fixed_size, size, &expr, 0, RELOC_TYPE_DEFAULT);
            frag_increase_fixed_size (size);
        } else {
            as_error ("value is not a constant");
            return;
        }
    } while (*((*pp)++) == ',');
    
    demand_empty_rest_of_line (pp);
}

static void handler_align (char **pp, int first_arg_is_bytes)
{
    offset_t alignment;
    int fill_specified;
    offset_t fill_value = 0;
    offset_t max_bytes_to_skip;
    
    alignment = get_result_of_absolute_expression (pp);

    if (first_arg_is_bytes && alignment != 0) {
        offset_t i;

        /* Converts to log2. */    
        for (i = 0; (alignment & 1) == 0; alignment >>= 1, i++);
        
        if (alignment != 1) {
            as_error ("alignment is not a power of 2!");
        }
        
        alignment = i;
    }

    if (**pp != ',') {

        fill_specified = 0;
        max_bytes_to_skip = 0;

    } else {

        ++*pp;
        *pp = skip_whitespace (*pp);

        if (**pp == ',') {
            fill_specified = 0;
        } else {
            
            fill_value = get_result_of_absolute_expression (pp);
            fill_specified = 1;
            
        }

        if (**pp != ',') {
            max_bytes_to_skip = 0;
        } else {

            ++*pp;
            max_bytes_to_skip = get_result_of_absolute_expression (pp);

        }

    }
    
    if (fill_specified) {
        frag_align (alignment, fill_value, max_bytes_to_skip);
    } else {
        if (section_get_flags (current_section) & SECTION_FLAG_CODE) {
            frag_align_code (alignment, max_bytes_to_skip);
        } else {
            frag_align (alignment, 0, max_bytes_to_skip);
        }
    }

    section_record_alignment_power (current_section, alignment);
    
    demand_empty_rest_of_line (pp);
}

static void handler_align_bytes (char **pp) {
    handler_align (pp, 1 /* first_arg_is_bytes */);
}

static int read_and_append_char_in_ascii (char **pp) {

    char ch;
    int i;
    unsigned long number;
    
    switch (ch = *((*pp)++)) {
    
        case '\"':
        
            return 1;
        
        case '\0':
        
            as_warn ("null character in string; '\"' inserted");
            
            --*pp;                      /* Might be the end of line buffer. */
            return 1;
        
        case '\n':
        
            as_warn ("unterminated string; newline inserted");
            
            line_number++;
            frag_append_1_char (ch);
            
            break;
        
        case '\\':
        
            ch = *((*pp)++);
            
            switch (ch) {
            
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                
                    for (i = 0, number = 0; isdigit ((unsigned char)ch) && (i < 3); (ch = *((*pp)++)), i++) {
                        number = number * 8 + ch - '0';
                    }
                    
                    frag_append_1_char (number & 0xff);
                    (*pp)--;
                    
                    break;

                case 'x':
                case 'X':
                    ch = *((*pp)++);
                    
                    for (i = 0, number = 0;
                         isxdigit ((unsigned char)ch) && (i < 3);
                         (ch = *((*pp)++)), i++) {
                        if (isdigit ((unsigned char)ch)) number = number * 16 + ch - '0';
                        else if (isupper ((unsigned char)ch)) number = number * 16 + ch - 'A' + 10;
                        else number = number * 16 + ch - 'a' + 10;
                    }
                    
                    frag_append_1_char (number & 0xff);
                    (*pp)--;
                    
                    break;
                
                case 'r':
                
                    frag_append_1_char (13);
                    break;
                
                case 'n':
                
                    frag_append_1_char (10);
                    break;
                
                case '\\':
                case '"':
                
                    frag_append_1_char (ch);
                    break;
                
                default:
                
                    as_internal_error_at_source (__FILE__, __LINE__, "+++UNIMPLEMENTED escape sequence read_and_append_char_in_ascii");
                    break;
            
            }
            
            break;
        
        default:
        
            frag_append_1_char (ch);
            break;
    
    }
    
    return 0;

}

static void handler_ascii (char **pp) {

    char ch;
    
    if ((ch = *((*pp)++)) == '"') {
    
        while (!read_and_append_char_in_ascii (pp)) {
            /* Nothing to do */
        }
    
    }
    
    demand_empty_rest_of_line (pp);

}

void handler_asciz (char **pp) {

    handler_ascii (pp);
    frag_append_1_char (0);

}

static void handler_byte (char **pp) {
    handler_constant (pp, 1, 0);
}

static void handler_comm (char **pp)
{
    struct expr expr;
    struct symbol *symbol;
    offset_t alignment = 0;
    
    char *name;
    char *name_end;
    char ch;
    
    *pp = skip_whitespace (*pp);
    name = *pp;
    
    ch = get_symbol_name_end (pp);
    
    if (name == *pp) {
        as_error ("expected symbol name");
        
        **pp = ch;
        ignore_rest_of_line (pp);
        
        return;
    }
    
    name_end = *pp;
    **pp = ch;
    
    *pp = skip_whitespace (*pp);
    
    if (**pp == ',') {
        (*pp)++;
    }
    
    absolute_expression_read_into (pp, &expr);
    
    if (expr.type == EXPR_TYPE_ABSENT) {
        as_error ("missing size expression");
        ignore_rest_of_line (pp);
        return;
    }
    
    *name_end = '\0';
    
    symbol = symbol_find_or_make (name);
    *name_end = ch;

    *pp = skip_whitespace (*pp);
    if (**pp == ',') {
        struct expr align_expr;
        
        ++*pp;
        alignment = absolute_expression_read_into (pp, &align_expr);
        
        if (align_expr.type == EXPR_TYPE_ABSENT) {
            as_error ("expected alignment after size");
            ignore_rest_of_line (pp);
            return;
        }

        if (alignment < 0) {
            as_warn ("alignment negative; 0 assumed");
            alignment = 0;
        }

        if (alignment != 0) {
            offset_t i;

            /* Converts to log2. */    
            for (i = 0; (alignment & 1) == 0; alignment >>= 1, i++);
            
            if (alignment != 1) {
                as_error ("alignment is not a power of 2!");
            }
            
            alignment = i;
        }
    }
    
    if (symbol_is_undefined (symbol)) {
        if (symbol->flags & SYMBOL_FLAG_LOCAL) {
            /* This is complicated way of doing .lcomm with alignment for ELF.
             * When ".local symbol" and then ".comm symbol, size, alignment"
             * is used, it should be translated into .lcomm.
             * But when the order is reversed (.comm, then .local),
             * regular .comm should be done...
             */
            section_t saved_section = current_section;
            subsection_t saved_subsection = current_subsection;
            
            section_subsection_set (bss_section, 1);

            if (alignment) {
                frag_align (alignment, 0, 0);
                section_record_alignment_power (current_section, alignment);
            }
            
            symbol->section = bss_section;
            symbol->frag = current_frag;
            
            symbol_set_value (symbol, current_frag->fixed_size);
            frag_increase_fixed_size (expr.add_number);
            
            section_subsection_set (saved_section, saved_subsection);
        } else {
            symbol_set_value (symbol, expr.add_number);
            symbol_set_external (symbol);
            if (alignment) {
                if (state->format == AS_FORMAT_ELF) {
                    elf_comm_symbol_align (symbol, alignment);
                } else {
                    as_warn (".comm alignment is supported only for ELF");
                }
            }
        }
    } else {
        as_error ("symbol '%s' is already defined", symbol->name);
        ignore_rest_of_line (pp);
        return;
    }
    
    demand_empty_rest_of_line (pp);
}

static void handler_data (char **pp) {

    section_subsection_set (data_section, (subsection_t) get_result_of_absolute_expression (pp));
    demand_empty_rest_of_line (pp);

}

static void internal_set (char **pp, struct symbol *symbol) {

    struct expr expr;
    
    machine_dependent_simplified_expression_read_into (pp, &expr);
    
    if (expr.type == EXPR_TYPE_INVALID) {
        as_error ("invalid expression");
    } else if (expr.type == EXPR_TYPE_ABSENT) {
        as_error ("missing expression");
    }
    
    if (symbol_is_section_symbol (symbol)) {
    
        as_error ("attempt to set value of section symbol");
        return;
    
    }
    
    switch (expr.type) {
    
        case EXPR_TYPE_INVALID:
        case EXPR_TYPE_ABSENT:
        
            expr.add_number = 0;
            /* fall through. */
        
        case EXPR_TYPE_CONSTANT:
        
            symbol_set_frag (symbol, &zero_address_frag);
            symbol_set_section (symbol, absolute_section);
            symbol_set_value (symbol, expr.add_number);
            break;
        
        default:
        
            symbol_set_frag (symbol, &zero_address_frag);
            symbol_set_section (symbol, expr_section);
            symbol_set_value_expression (symbol, &expr);
            break;
    
    }

}

static void assign_symbol (char **pp, char *name) {

    struct symbol *symbol;
    
    /* .equ ., some_expr is an alias for .org some_expr. */
    if (name[0] == '.' && name[1] == '\0') {
    
        struct expr expr;
        section_t section = get_known_section_expression (pp, &expr);
        
        do_org (section, &expr, 0);
        return;
    
    }
    
    symbol = symbol_find_or_make (name);
    internal_set (pp, symbol);

}

static void handler_equ (char **pp) {

    char *name;
    char *name_end;
    char ch;
    
    *pp = skip_whitespace (*pp);
    name = *pp;
    
    ch = get_symbol_name_end (pp);
    
    if (name == *pp) {
    
        as_error ("expected symbol name");
        
        **pp = ch;
        ignore_rest_of_line (pp);
        
        return;
    
    }
    
    name_end = *pp;
    **pp = ch;
    
    *pp = skip_whitespace (*pp);
    
    if (**pp != ',') {
    
        *name_end = '\0';
        
        as_error ("expected comma after \"%s\"", name);
        *name_end = ch;
        
        ignore_rest_of_line (pp);
        return;
    
    }
    
    (*pp)++;
    
    *name_end = '\0';
    assign_symbol (pp, name);
    
    *name_end = ch;
    demand_empty_rest_of_line (pp);

}

static void handler_global (char **pp) {
    
    for (;;) {
    
        struct symbol *symbol;
        
        char *name;
        char ch;
        
        *pp = skip_whitespace (*pp);
        name = *pp;
        
        ch = get_symbol_name_end (pp);
        
        if (name == *pp) {
        
            as_error ("expected symbol name");
            
            **pp = ch;
            ignore_rest_of_line (pp);
            
            return;
        
        }
        
        symbol = symbol_find_or_make (name);
        symbol_set_external (symbol);
        
        **pp = ch;
        *pp = skip_whitespace (*pp);
        
        if (**pp != ',') {
            break;
        }
        
        ++*pp;
    
    }
    
    demand_empty_rest_of_line (pp);

}

void handler_ignore (char **pp) {
    ignore_rest_of_line (pp);
}

static void handler_include (char **pp) {

    char *orig_ilp;
    
    const char *orig_filename = filename;
    unsigned long orig_ln = line_number;
    
    char *p2 = NULL, *tmp;
    int len = 0;
    
    int i;
    
    *pp = skip_whitespace (*pp);
    tmp = *pp;
    
    if (**pp == '"') {
    
        char ch;
        
        while ((ch = *++*pp)) {
        
            if (ch == '"') {
            
                ++*pp;
                break;
            
            }
            
            len++;
            continue;
        
        }
        
        if (ch != '"') {
        
            as_error ("unterminated string");
            
            ignore_rest_of_line (pp);
            return;
        
        }
        
    } else {
        
        as_error ("missing string");
        
        ignore_rest_of_line (pp);
        return;
    
    }
    
    p2 = xmalloc (len + 1);
    tmp++;
    
    for (i = 0; i < len; i++) {
    
        if (*tmp == '"') {
            break;
        }
        
        p2[i] = *tmp++;
    
    }
    
    p2[len] = '\0';
    
    demand_empty_rest_of_line (pp);
    orig_ilp = *pp;
    
    if (!process (p2)) {
    
        *pp = orig_ilp;
        goto done;
    
    }
    
    for (i = 0; i < state->nb_include_paths; i++) {
    
        tmp = xmalloc (strlen (state->include_paths[i]) + len);
        
        strcpy (tmp, state->include_paths[i]);
        strcat (tmp, p2);
        
        if (!process (tmp)) {
        
            free (tmp);
            
            *pp = orig_ilp;
            goto done;
        
        }
        
        free (tmp);
    
    }
    
    as_error_at (orig_filename, orig_ln, "can't open '%s' for reading", p2);

done:

    free (p2);
    
    filename = orig_filename;
    line_number = orig_ln;

}

void handler_lcomm (char **pp)
{
    struct expr expr;
    struct symbol *symbol;
    offset_t alignment = 0;
    
    char *name;
    char *name_end;
    char ch;
    
    section_t saved_section = current_section;
    subsection_t saved_subsection = current_subsection;
    
    *pp = skip_whitespace (*pp);
    name = *pp;
    
    ch = get_symbol_name_end (pp);
    
    if (name == *pp) {
    
        as_error ("expected symbol name");
        
        **pp = ch;
        ignore_rest_of_line (pp);
        
        return;
    
    }
    
    name_end = *pp;
    **pp = ch;
    
    *pp = skip_whitespace (*pp);
    
    if (**pp == ',') {
        (*pp)++;
    }
    
    absolute_expression_read_into (pp, &expr);
    
    if (expr.type == EXPR_TYPE_ABSENT) {
    
        as_error ("missing size expression");
        
        ignore_rest_of_line (pp);
        return;
    
    }
    
    *name_end = '\0';
    
    symbol = symbol_find_or_make (name);
    *name_end = ch;

    *pp = skip_whitespace (*pp);
    if (**pp == ',') {

        struct expr align_expr;
        
        ++*pp;

        alignment = absolute_expression_read_into (pp, &align_expr);

        if (align_expr.type == EXPR_TYPE_ABSENT) {

            as_error ("expected alignment after size");
            ignore_rest_of_line (pp);
            return;

        }

        if (alignment < 0) {

            as_warn ("alignment negative; 0 assumed");
            alignment = 0;

        }

        if (alignment != 0) {

            offset_t i;

            /* Converts to log2. */    
            for (i = 0; (alignment & 1) == 0; alignment >>= 1, i++);
            
            if (alignment != 1) {
                as_error ("alignment is not a power of 2!");
            }
            
            alignment = i;

        }

    }   
    
    if (symbol_is_undefined (symbol)) {
        section_subsection_set (bss_section, 1);

        if (alignment) {
            frag_align (alignment, 0, 0);
            section_record_alignment_power (current_section, alignment);
        }
        
        symbol->section = bss_section;
        symbol->frag = current_frag;
        
        symbol_set_value (symbol, current_frag->fixed_size);
        frag_increase_fixed_size (expr.add_number);
        
        section_subsection_set (saved_section, saved_subsection);
    } else {
        as_error ("symbol '%s' is already defined", symbol->name);
        
        ignore_rest_of_line (pp);
        return;
    }
    
    demand_empty_rest_of_line (pp);  
}

static void handler_linkonce (char **pp)
{
    flag_int flags;

    flags = section_get_flags (current_section);
    flags |= SECTION_FLAG_LINK_ONCE;

    if (!is_end_of_line[(int) **pp]) {
        char *name;
        char ch;

        name = *pp;
        ch = get_symbol_name_end (pp);

        if (xstrcasecmp (name, "discard") == 0) {
            flags |= SECTION_FLAG_LINK_DUPLICATES_DISCARD;
        } else if (xstrcasecmp (name, "one_only") == 0) {
            flags |= SECTION_FLAG_LINK_DUPLICATES_ONE_ONLY;
        } else if (xstrcasecmp (name, "same_size") == 0) {
            flags |= SECTION_FLAG_LINK_DUPLICATES_SAME_SIZE;
        } else if (xstrcasecmp (name, "same_contents") == 0) {
            flags |= SECTION_FLAG_LINK_DUPLICATES_SAME_CONTENTS;
        } else {
            as_warn ("unrecognized .linkonce type '%s'", name);
        }
        
        **pp = ch;
    } else flags |= SECTION_FLAG_LINK_DUPLICATES_DISCARD;
    
    if (state->format != AS_FORMAT_COFF) {
        as_warn (".linkonce is not supported for this object file format");
    }

    section_set_flags (current_section, flags);
    
    demand_empty_rest_of_line (pp);
}

static void handler_long (char **pp) {
    handler_constant (pp, 4, 0);
}

static void do_org (section_t section, struct expr *expr, unsigned long fill_value) {

    struct symbol *symbol;
    
    unsigned long offset;
    unsigned char *p_in_frag;
    
    if (section != current_section && section != absolute_section && section != expr_section) {
        as_error ("invalid section \"%s\"", section_get_name (section));
    }
    
    symbol = expr->add_symbol;
    offset = expr->add_number;
    
    if (fill_value && current_section == bss_section) {
        as_warn ("ignoring fill value in section \"%s\"", section_get_name (current_section));
    }
    
    if (expr->type != EXPR_TYPE_CONSTANT && expr->type != EXPR_TYPE_SYMBOL) {
    
        symbol = make_expr_symbol (expr);
        offset = 0;
    
    }
    
    p_in_frag = frag_alloc_space (1);
    *p_in_frag = (unsigned char) fill_value;
    
    frag_set_as_variant (RELAX_TYPE_ORG, 0, symbol, offset, 0);

}

static void handler_org (char **pp) {

    struct expr expr;
    section_t section;
    
    unsigned long fill_value;
    
    section = get_known_section_expression (pp, &expr);
    
    if (**pp == ',') {
    
        ++*pp;
        
        as_internal_error_at_source (__FILE__, __LINE__, NULL, 0, "+++handler_org");
        fill_value = 0;
    
    } else {
        fill_value = 0;
    }
    
    do_org (section, &expr, fill_value);
    demand_empty_rest_of_line (pp);

}

static void handler_p2align (char **pp) {
    handler_align (pp, 0 /* first_arg_is_bytes */);
}

static void handler_quad (char **pp) {
    handler_constant (pp, 8, 0);
}

static void handler_rva (char **pp) {
    handler_constant (pp, 4, 1);
}

static void handler_space (char **pp)
{
    struct expr expr, val;
    offset_t repeat;
    
    machine_dependent_simplified_expression_read_into (pp, &expr);
    
    if (**pp == ',') {
        ++(*pp);
        machine_dependent_simplified_expression_read_into (pp, &val);
        
        if (val.type != EXPR_TYPE_CONSTANT) {
            as_error ("invalid value for .space");
            
            ignore_rest_of_line (pp);
            return;
        }

        if (val.add_number > 0xff) {
            as_warn ("fill value %#"PRIxVALUE" truncated to %#"PRIxVALUE,
                     val.add_number,
                     val.add_number & 0xff);
            val.add_number &= 0xff;
        }
    } else {
        val.type = EXPR_TYPE_CONSTANT;
        val.add_number = 0;
    }
    
    if (expr.type == EXPR_TYPE_CONSTANT) {
        repeat = expr.add_number;
        
        if (repeat == 0) {
            as_warn (".space repeat count is zero, ignored");
            goto end;
        }
        
        if (repeat < 0) {
            as_warn (".space repeat count is negative, ignored");
            goto end;
        }

        memset (frag_increase_fixed_size (repeat), val.add_number, repeat);
    } else {
        struct symbol *expr_symbol = make_expr_symbol (&expr);
        
        unsigned char *p = frag_alloc_space (symbol_get_value (expr_symbol));
        *p = val.add_number;
        
        frag_set_as_variant (RELAX_TYPE_SPACE, 0, expr_symbol, 0, 0);
    }
    
    if ((val.type != EXPR_TYPE_CONSTANT || val.add_number != 0) && current_section == bss_section) {
        as_warn ("ignoring fill value in section '%s'", section_get_name (current_section));
    }
    
end:
    demand_empty_rest_of_line (pp);
}

static void handler_text (char **pp) {

    section_subsection_set (text_section, (subsection_t) get_result_of_absolute_expression (pp));
    demand_empty_rest_of_line (pp);

}

static void handler_word (char **pp) {
    handler_constant (pp, 2, 0);
}

static struct pseudo_op_entry pseudo_op_table[] = {

    { "align",      &handler_align_bytes    },
    { "ascii",      &handler_ascii          },
    { "asciz",      &handler_asciz          },
    { "balign",     &handler_align_bytes    },
    { "byte",       &handler_byte           },
    { "comm",       &handler_comm           },
    { "data",       &handler_data           },
    { "else",       &cond_handler_else      },
    { "endif",      &cond_handler_endif     },
    { "equ",        &handler_equ            },
    { "file",       &handler_ignore         },
    { "globl",      &handler_global         },
    { "global",     &handler_global         },
    { "if",         &cond_handler_if        },
    { "ifdef",      &cond_handler_ifdef     },
    { "ifndef",     &cond_handler_ifndef    },
    { "include",    &handler_include        },
    { "lcomm",      &handler_lcomm          },
    { "linkonce",   &handler_linkonce       },
    { "long",       &handler_long           },
    { "org",        &handler_org            },
    { "p2align",    &handler_p2align        },
    { "quad",       &handler_quad           },
    { "rva",        &handler_rva            },
    { "set",        &handler_equ            },
    { "skip",       &handler_space          },
    { "space",      &handler_space          },
    { "string",     &handler_asciz          },
    { "text",       &handler_text           },
    { "value",      &handler_word           },
    { "word",       &handler_word           },
    { "zero",       &handler_space          },
    { 0,            0                       }

};

static hash_value_t hash_pseudo_op_entry (const void *p) {

    const struct pseudo_op_entry *entry = (const struct pseudo_op_entry *) p;
    return hashtab_help_default_hash_string (entry->name);

}

static int equal_pseudo_op_entries (const void *p1, const void *p2) {

    const struct pseudo_op_entry *entry1 = (const struct pseudo_op_entry *) p1;
    const struct pseudo_op_entry *entry2 = (const struct pseudo_op_entry *) p2;
    
    return strcmp (entry1->name, entry2->name) == 0;

}

static struct pseudo_op_entry *pseudo_op_find (const char *name) {

    struct pseudo_op_entry fake = { NULL, NULL };
    fake.name = name;
    
    return (struct pseudo_op_entry *) hashtab_find (pseudo_op_hashtab, &fake);

}

static void install_pseudo_op_table (struct pseudo_op_entry *table) {

    struct pseudo_op_entry *entry;
    
    for (entry = table; entry->name; entry++) {
    
        if (pseudo_op_find (entry->name)) {
            continue;
        }
        
        if (hashtab_insert (pseudo_op_hashtab, entry)) {
            as_internal_error_at_source_at (__FILE__, __LINE__, NULL, 0, "error constructing pseudo-op table");
        }
    
    }

}

void process_init (void) {

    const char *p;
    
    pseudo_op_hashtab = hashtab_create_hashtab (0, hash_pseudo_op_entry, equal_pseudo_op_entries, &xmalloc, &free);
    
    if (pseudo_op_hashtab == NULL) {
        as_internal_error_at_source_at (__FILE__, __LINE__, NULL, 0, "error creating pseudo_op_hashtab");
    }
    
    /* Machine dependent psedo-ops. */
    install_pseudo_op_table (machine_dependent_get_pseudo_op_table ());
    
    /* Object dependent pseudo-ops. */
    switch (state->format) {
    
        case AS_FORMAT_A_OUT:
        
            install_pseudo_op_table (a_out_get_pseudo_op_table ());
            break;
        
        case AS_FORMAT_COFF:
        
            install_pseudo_op_table (coff_get_pseudo_op_table ());
            break;

        case AS_FORMAT_ELF:
        
            install_pseudo_op_table (elf_get_pseudo_op_table ());
            break;
    
    }
    
    /* Standard pseudo-ops. */
    install_pseudo_op_table (pseudo_op_table);

    /* CFI pseudo-ops last. */
    install_pseudo_op_table (cfi_get_pseudo_op_table ());
    
    /* Installs machine dependent line separators. */
    for (p = machine_dependent_get_line_separators (); *p; p++) {
        is_end_of_line[(int) *p] = 2;
    }
    
    /* Installs machine dependent start-of-line comment characters. */
    for (p = machine_dependent_get_comment_at_the_start_of_line_beginners (); *p; p++) {
        is_comment_at_the_start_of_line[(int) *p] = 1;
    }

}

void process_destroy (void)
{
    hashtab_destroy_hashtab (pseudo_op_hashtab);
}

char get_symbol_name_end (char **pp) {

    char c = **pp;
    
    if (is_name_beginner ((int) (*pp)[0])) {
    
        while (is_name_part ((int) (*pp)[0])) {
            (*pp)++;
        }
        
        c = **pp;
    
    }
    **pp = '\0';
    
    return c;

}

#define HANDLE_CONDITIONAL() \
 if (cond_can_ignore_line (line)) { \
     line = line_end; \
     continue; \
 }

int process (const char *fname)
{
    FILE *input;
    
    char *line;
    char *line_end;
    
    char *real_line;
    size_t real_line_len;
    
    unsigned long newlines;
    unsigned long new_line_number;
    
    void *load_line_internal_data = NULL;
    
    filename = fname;
    
    if ((input = fopen (fname, "r")) == NULL) {
        return 1;
    }
    
    line_number = 0;
    new_line_number = 1;
    
    load_line_internal_data = load_line_create_internal_data (&new_line_number);
    
    while (!load_line (&line, &line_end, &real_line, &real_line_len, &newlines, input, &load_line_internal_data)) {
    
        line_number = new_line_number;
        new_line_number += newlines + 1;
        
        if (state->generate_listing) {
            update_listing_line (current_frag);
            add_listing_line (real_line, real_line_len, filename, line_number);
        }
        
        while (line < line_end) {
            char *start_p;
            
            line = skip_whitespace (line);
            start_p = line;
            
            if (is_name_beginner ((int) *line)) {
                char saved_c;
                struct pseudo_op_entry *poe = NULL;

                HANDLE_CONDITIONAL ();
                
                saved_c = get_symbol_name_end (&line);
                
                if (saved_c && (saved_c == ':' || (*skip_whitespace (line + 1)) == ':')) {
                    symbol_label (start_p);
                    
                    *line = saved_c;
                    
                    if (*line != ':') {
                        line = skip_whitespace (line + 1) + 1;
                    }
                    
                    line = skip_whitespace (line + 1);
                    
                    continue;
                }

                {
                    char *p;

                    for (p = start_p; *p; p++) {
                        *p = tolower ((unsigned char)*p);
                    }
                }

                if (state->no_pseudo_dot) {
                    poe = pseudo_op_find (start_p);
                }
                
                if (poe || *start_p == '.') {
                    if (!poe) poe = pseudo_op_find (start_p + 1);
                    
                    if (poe == NULL) {
                        as_error ("unknown pseudo-op '%s'", start_p);
                        
                        *line = saved_c;
                        handler_ignore (&line);
                        continue;
                    }
                    
                    *line = saved_c;
                    line = skip_whitespace (line);
                    
                    (*(poe->handler)) (&line);
                    continue;
                }
                
                *line = saved_c;
                
                saved_c = *(line = find_end_of_line (line));
                *line = '\0';
                
                machine_dependent_assemble_line (skip_whitespace (start_p));
                
                *(line++) = saved_c;
                
                continue;
            }
            
            if (is_end_of_line[(int) *line]) {
                line++;
                continue;
            }
            
            if (is_comment_at_the_start_of_line[(int) *line]) {
                break;
            }

            HANDLE_CONDITIONAL ();
            
            demand_empty_rest_of_line (&line);
        }
    }

    cond_end_of_file ();
    
    if (state->generate_listing) {
        update_listing_line (current_frag);
    }
    
    load_line_destroy_internal_data (load_line_internal_data);
    
    fclose (input);
    return 0;
}

void demand_empty_rest_of_line (char **pp) {

    *pp = skip_whitespace (*pp);
    
    if (is_end_of_line[(int) **pp]) {
        ++*pp;
    } else {
    
        if (isprint ((int) **pp)) {
            as_error ("junk at the end of line, first unrecognized character is '%c'", **pp);
        } else {
            as_error ("junk at the end of line, first unrecognized character valued 0x%x", **pp);
        }
        
        ignore_rest_of_line (pp);
    
    }

}

void get_filename_and_line_number (const char **filename_p, unsigned long *line_number_p) {

    *filename_p = filename;
    *line_number_p = line_number;
    
}

void ignore_rest_of_line (char **pp) {

    *pp = find_end_of_line (*pp);
    ++*pp;

}

char *find_end_of_line (char *line)
{
    while (!is_end_of_line[(int) *line]) {
        if (line[0] == '\"') {
            line++;
            while (*line && *line != '\"') {
                if (line++[0] == '\\' && *line) {
                    line++;
                }
            }
        } else if (line[0] == '\'') {
            if (is_end_of_line[(int)line[1]] != 1) {
                if (line[2] == '\'') line++;
                line++;
            }
        }

        line++;
    }
    
    return line;
}
