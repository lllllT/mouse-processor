/*
 * s_exp_assq.c  -- s-expression assq
 *
 * $Id: s_exp_assq.c,v 1.3 2005/07/28 09:41:18 hos Exp $
 *
 */

#include "s_exp.h"

s_exp_data_t *s_exp_assq_get(const s_exp_data_t *alist, const wchar_t *sym)
{
    const s_exp_data_t *d;
    s_exp_data_t *s;

    s = s_exp_intern(sym);
    if(S_EXP_ERROR(s)) {
        return NULL;
    }

    S_EXP_FOR_EACH(alist, d) {
        if(S_EXP_CAR(d)->type == S_EXP_TYPE_CONS && S_EXP_CAAR(d) == s) {
            free_s_exp(s);
            return S_EXP_CAR(d);
        }
    }
    free_s_exp(s);

    return NULL;
}

s_exp_data_t *s_exp_assq(const s_exp_data_t *alist, const wchar_t *sym)
{
    s_exp_data_t *p = s_exp_assq_get(alist, sym);
    if(p == NULL) {
        return NULL;
    }

    return S_EXP_CDR(p);
}
