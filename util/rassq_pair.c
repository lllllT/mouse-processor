/*
 * rassq_pair.c  -- utility function rassq_pair
 *
 * $Id: rassq_pair.c,v 1.1 2005/01/05 07:59:35 hos Exp $
 *
 */


#include "util.h"


unsigned long rassq_pair(const struct uint_ptr_pair *pair,
                         void *ptr, unsigned long default_val)
{
    while(pair->ptr != (void *)0) {
        if(pair->ptr == ptr) {
            return pair->key;
        }

        pair++;
    }

    return default_val;
}
