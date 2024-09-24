/******************************************************************************
 * @file            conv.c
 *****************************************************************************/
#include    "stdint.h"

uint32_t conv_dec (char *str, int32_t max) {

    uint32_t value = 0;
    
    while (*str != ' ' && max-- > 0) {
    
        value *= 10;
        value += *str++ - '0';
    
    }
    
    return value;

}
