/*
 * scroll_op.c  -- scroll operators
 *
 * $Id: scroll_op.c,v 1.4 2005/01/13 09:39:56 hos Exp $
 *
 */

#include "main.h"
#include "scroll_op.h"
#include "scroll_op_utils.h"
#include <math.h>


wchar_t *get_scroll_op_mode_name(const s_exp_data_t *arg,
                                 const s_exp_data_t *conf,
                                 const wchar_t *def_val)
{
    const s_exp_data_t *sym;

    if((sym = s_exp_nth(arg, 0)) != NULL &&
       sym->type == S_EXP_TYPE_SYMBOL) {
        return sym->symbol.name;
    } else if((sym = s_exp_massq(conf, S_EXP_TYPE_SYMBOL,
                                 L"default-mode", NULL)) != NULL) {
        return sym->symbol.name;
    } else {
        return (wchar_t *)def_val;
    }
}

int get_scroll_op_mode(struct scroll_op_mode_pair *mode_map,
                       const wchar_t *mode_name)
{
    int i, mode;

    mode = -1;
    for(i = 0; mode_map[i].name != NULL; i++) {
        if(wcscmp(mode_name, mode_map[i].name) == 0) {
            mode = mode_map[i].mode;
            break;
        }
    }

    return mode;
}

void get_scroll_op_xy_ratio(const s_exp_data_t *arg,
                            const s_exp_data_t *mode_conf,
                            double *x_ratio, double *y_ratio,
                            double def_x_ratio, double def_y_ratio)
{
    s_exp_data_t *xr, *yr;

    if((xr = s_exp_nth(arg, 0)) != NULL ||
       (xr = s_exp_nth(mode_conf, 0)) != NULL) {
        if(xr->type == S_EXP_TYPE_INTEGER) {
            *x_ratio = xr->number.val;
        } else if(xr->type == S_EXP_TYPE_FLONUM) {
            *x_ratio = xr->flonum.val;
        } else {
            *x_ratio = def_x_ratio;
        }
    }

    if((yr = s_exp_nth(arg, 1)) != NULL ||
       (yr = s_exp_nth(mode_conf, 1)) != NULL) {
        if(yr->type == S_EXP_TYPE_INTEGER) {
            *y_ratio = yr->number.val;
        } else if(yr->type == S_EXP_TYPE_FLONUM) {
            *y_ratio = yr->flonum.val;
        } else {
            *y_ratio = def_y_ratio;
        }
    }
}


int get_drag_scroll_delta(int length,
                          int min, int max, double page_ratio,
                          double delta, double *rest)
{
    int range;
    int ret;
    double spd;

    range = max - min + 1;
    spd = (page_ratio >= 1 ? 0 : (range / (length * (1 - page_ratio))));

    ret = trunc(delta * spd);
    *rest = delta - ret / spd;
    return ret;
}

