/*
 * util.h  -- definition of utility
 *
 * $Id: util.h,v 1.1 2005/01/05 07:46:32 hos Exp $
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__


struct uint_ptr_pair {
    unsigned long key;
    void *ptr;
};

void *assq_pair(const struct uint_ptr_pair *pair,
                unsigned long key, void *default_ptr);


#endif /* __UTIL_H__ */
