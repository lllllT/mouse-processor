/*
 * s_exp_nth.c  -- nth element of list
 *
 */

#include "s_exp.h"

s_exp_data_t *s_exp_nth(const s_exp_data_t *list, int nth)
{
    const s_exp_data_t *p;
    int n;

    n = 0;
    S_EXP_FOR_EACH(list, p) {
        if(n == nth) {
            return S_EXP_CAR(p);
        }

        if(n > nth) {
            break;
        }

        n += 1;
    }

    return NULL;
}
