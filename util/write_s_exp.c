/*
 * write_s_exp.c  -- write s-expression
 *
 * $Id: write_s_exp.c,v 1.6 2005/01/19 08:36:50 hos Exp $
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "s_exp.h"
#include "util.h"


int write_s_exp(FILE *fp, const s_exp_data_t *data)
{
    unsigned char *str;

    str = u8s_write_s_exp(data);
    if(str == NULL) {
        return 0;
    }

    fprintf(fp, "%s", str);

    free(str);

    return 1;
}
