/*
 * scroll_op.c  -- scroll operators
 *
 * $Id: scroll_op.c,v 1.10 2005/01/21 04:38:15 hos Exp $
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


struct or_context {
    struct scroll_operator_conf *op;
    int op_context_size;
    void *op_context;
};

static
int SCROLL_OP_API or_get_ctx_size(const scroll_op_arg_t *arg)
{
    return sizeof(struct or_context);
}

static
int SCROLL_OP_API or_init_ctx(void *ctxp, int size,
                              const scroll_op_arg_t *arg)
{
    struct or_context *op_ctx;
    s_exp_data_t *p;
    wchar_t *name;
    int i;

    if(size != sizeof(struct or_context)) {
        return 0;
    }

    op_ctx = (struct or_context *)ctxp;

    /* search and apply operator initializer */
    S_EXP_FOR_EACH(arg->arg, p) {
        scroll_op_arg_t op_arg;
        struct scroll_operator_conf *op;

        if(S_EXP_CAR(p)->type != S_EXP_TYPE_CONS ||
           S_EXP_CAAR(p)->type != S_EXP_TYPE_SYMBOL) {
            continue;
        }

        /* operator name */
        name = S_EXP_CAAR(p)->symbol.name;

        /* search operator */
        op = NULL;
        for(i = 0; i < ctx.app_conf.scroll_operator_num; i++) {
            if(wcscmp(name, ctx.app_conf.scroll_operator_conf[i].name) == 0) {
                op = &ctx.app_conf.scroll_operator_conf[i];
                break;
            }
        }
        if(op == NULL) {
            continue;
        }

        /* arg for operator */
        memset(&op_arg, 0, sizeof(op_arg));
        op_arg.conf = op->conf;
        op_arg.arg = S_EXP_CDAR(p);
        op_arg.hwnd = arg->hwnd;
        op_arg.pos = arg->pos;

        /* get size of operator context */
        if(op->proc.get_context_size != NULL) {
            op_ctx->op_context_size = op->proc.get_context_size(&op_arg);
            if(op_ctx->op_context_size < 0) {
                continue;
            }
        } else {
            op_ctx->op_context_size = 0;
        }

        /* allocate */
        op_ctx->op_context = malloc(op_ctx->op_context_size);
        if(op_ctx->op_context == NULL) {
            continue;
        }

        memset(op_ctx->op_context, 0, sizeof(op_ctx->op_context_size));

        /* initialize operator context */
        if(op->proc.init_context != NULL) {
            if(op->proc.init_context(op_ctx->op_context,
                                     op_ctx->op_context_size,
                                     &op_arg) == 0) {
                free(op_ctx->op_context);
                op_ctx->op_context = NULL;
                continue;
            }
        }

        op_ctx->op = op;

        if(wcscmp(op_ctx->op->name, L"or") != 0) {
            log_printf(LOG_LEVEL_DEBUG, L"scroll operator: ");
            log_print_s_exp(LOG_LEVEL_DEBUG, S_EXP_CAR(p));
            log_printf(LOG_LEVEL_DEBUG, L"\n");
        }

        return 1;
    }

    return 0;
}

static
int SCROLL_OP_API or_scroll(void *ctxp, double dx, double dy)
{
    struct or_context *op_ctx;

    op_ctx = (struct or_context *)ctxp;

    if(op_ctx->op == NULL ||
       op_ctx->op->proc.scroll == NULL) {
        return 0;
    }

    return op_ctx->op->proc.scroll(op_ctx->op_context, dx, dy);
}

static
int SCROLL_OP_API or_end_scroll(void *ctxp)
{
    struct or_context *op_ctx;
    int ret;

    op_ctx = (struct or_context *)ctxp;

    if(op_ctx->op != NULL &&
       op_ctx->op->proc.end_scroll != NULL) {
        ret = op_ctx->op->proc.end_scroll(op_ctx->op_context);
    }

    if(op_ctx->op_context != NULL)
        free(op_ctx->op_context);

    op_ctx->op_context = NULL;

    return ret;
}

static
int SCROLL_OP_API or_get_operator(scroll_op_procs_t *op,
                                  int api_ver)
{
    if(api_ver < SCROLL_OP_API_VERSION) {
        return 0;
    }

    op->get_context_size = or_get_ctx_size;
    op->init_context = or_init_ctx;
    op->scroll = or_scroll;
    op->end_scroll = or_end_scroll;

    return 1;
}


struct scroll_operator_def builtin_scroll_op[] = {
    {L"window-scrollbar", window_scrollbar_get_operator},
    {L"neighborhood-scrollbar", neighborhood_scrollbar_get_operator},
    {L"scrollbar-control", scrollbar_control_get_operator},
    {L"trackbar-control", trackbar_control_get_operator},
    {L"ie-scroll", ie_scroll_get_operator},
    {L"wheel-message", wheel_message_get_operator},

    {L"or", or_get_operator},

    {NULL, NULL}
};
