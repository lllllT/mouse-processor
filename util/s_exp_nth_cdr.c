/*
 * s_exp_nth_cdr.c  -- nth cdr of list
 *
 */

#include "s_exp.h"

s_exp_data_t *s_exp_nth_cdr(const s_exp_data_t *list, int nth)
{
    const s_exp_data_t *p;
    int n;

    n = 0;
    S_EXP_FOR_EACH(list, p) {
        if(n == nth) {
            return S_EXP_CDR(p);
        }

        if(n > nth) {
            break;
        }

        n += 1;
    }

    return S_EXP_NIL;
}
