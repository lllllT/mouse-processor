/*
 * write_s_exp.c  -- write s-expression
 *
 * $Id: write_s_exp.c,v 1.4 2005/01/14 14:54:38 hos Exp $
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "s_exp.h"
#include "util.h"


static unsigned char *u8s_write_list(const s_exp_data_t *data)
{
    unsigned char *car, *cdr, *str;

    car = u8s_write_s_exp(S_EXP_CAR(data));
    if(car == NULL) {
        return NULL;
    }

    if(S_EXP_CDR(data)->type == S_EXP_TYPE_CONS) {
        cdr = u8s_write_list(S_EXP_CDR(data));
        if(cdr == NULL) {
            free(car);
            return NULL;
        }

        str = (unsigned char *)malloc(strlen(car) + strlen(cdr) + 2);
        if(str == NULL) {
            free(car);
            free(cdr);
            return NULL;
        }

        strcpy(str, car);
        strcat(str, " ");
        strcat(str, cdr);

        free(car);
        free(cdr);

        return str;
    } else if(S_EXP_CDR(data) == S_EXP_NIL) {
        return car;
    } else {
        cdr = u8s_write_list(S_EXP_CDR(data));
        if(cdr == NULL) {
            free(car);
            return NULL;
        }

        str = (unsigned char *)malloc(strlen(car) + strlen(cdr) + 4);
        if(str == NULL) {
            free(car);
            free(cdr);
            return NULL;
        }

        strcpy(str, car);
        strcat(str, " . ");
        strcat(str, cdr);

        free(car);
        free(cdr);

        return str;
    }
}

unsigned char *u8s_write_s_exp(const s_exp_data_t *data)
{
    switch(data->type) {
      case S_EXP_TYPE_TRUE:
          return strdup("#t");

      case S_EXP_TYPE_FALSE:
          return strdup("#f");

      case S_EXP_TYPE_NIL:
          return strdup("()");

      case S_EXP_TYPE_CONS:
      {
          unsigned char *ls;
          unsigned char *str;

          ls = u8s_write_list(data);
          if(ls == NULL) {
              return NULL;
          }

          str = (unsigned char *)malloc(strlen(ls) + 3);
          if(str == NULL) {
              free(ls);
              return NULL;
          }

          strcpy(str, "(");
          strcat(str, ls);
          strcat(str, ")");

          free(ls);

          return str;
      }

      case S_EXP_TYPE_INTEGER:
      {
          unsigned char buf[32];
          sprintf(buf, "%d", data->number.val);
          return strdup(buf);
      }

      case S_EXP_TYPE_FLONUM:
      {
          unsigned char buf[32];
          sprintf(buf, "%lf", data->flonum.val);
          return strdup(buf);
      }

      case S_EXP_TYPE_STRING:
      {
          unsigned char *u8s, *str;

          u8s = u8s_dup_from_wcs(data->string.str);
          if(u8s == NULL) {
              return 0;
          }

          str = (unsigned char *)malloc(strlen(u8s) + 3);
          if(str == NULL) {
              free(u8s);
              return NULL;
          }

          strcpy(str, "\"");
          strcat(str, u8s);
          strcat(str, "\"");

          free(u8s);

          return str;
      }

      case S_EXP_TYPE_SYMBOL:
          return u8s_dup_from_wcs(data->symbol.name);

      case S_EXP_TYPE_ERROR:
      {
          unsigned char *str;

          str = (unsigned char *)malloc(strlen(data->error.descript) + 10);
          if(str == NULL) {
              return NULL;
          }

          strcpy(str, "#<error ");
          strcat(str, data->error.descript);
          strcat(str, ">");

          return str;
      }

      default:
          return NULL;
    }
}

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
