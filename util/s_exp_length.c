/*
 * s_exp_length.c  -- length of list
 *
 * $Id: s_exp_length.c,v 1.2 2005/01/06 08:49:05 hos Exp $
 *
 */

#include "s_exp.h"

int s_exp_length(const s_exp_data_t *list)
{
    int n;
    const s_exp_data_t *p;

    if(list == S_EXP_NIL) {
        return 0;
    }

    if(list->type != S_EXP_TYPE_CONS) {
        return -1;
    }

    n = 0;
    S_EXP_FOR_EACH(list, p) {
        n++;
    }

    return n;
}
