/*
 * s_exp_massq.c  -- s-expression multi-assq
 *
 */

#include "s_exp.h"
#include <stdarg.h>

s_exp_data_t *s_exp_massq_v(const s_exp_data_t *alist, int type, va_list ap)
{
    const wchar_t *sn;
    const s_exp_data_t *d;

    d = alist;
    while(1) {
        sn = va_arg(ap, const wchar_t *);
        if(sn == NULL) {
            if(d->type != type) {
                return NULL;
            }

            break;
        }

        d = s_exp_assq(d, sn);
        if(d == NULL) {
            return NULL;
        }
    }

    return (s_exp_data_t *)d;
}

s_exp_data_t *s_exp_massq(const s_exp_data_t *alist, int type, ...)
{
    va_list ap;
    const s_exp_data_t *ret;

    va_start(ap, type);
    ret = s_exp_massq_v(alist, type, ap);
    va_end(ap);

    return (s_exp_data_t *)ret;
}
