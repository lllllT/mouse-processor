/*
 * scroll_op_meta.c  -- meta scroll operator
 *
 */

#include "main.h"
#include "operator.h"


static const support_procs_t *spr = NULL;


struct meta_context {
    struct scroll_operator_conf *op;
    int op_context_size;
    void *op_context;
};

static
int MP_OP_API meta_get_ctx_size(const op_arg_t *arg)
{
    return sizeof(struct meta_context);
}

static
int MP_OP_API meta_scroll(void *ctxp, double dx, double dy)
{
    struct meta_context *op_ctx;

    op_ctx = (struct meta_context *)ctxp;

    if(op_ctx->op == NULL ||
       op_ctx->op->proc.scroll == NULL) {
        return 0;
    }

    return op_ctx->op->proc.scroll(op_ctx->op_context, dx, dy);
}

static
int MP_OP_API meta_end_scroll(void *ctxp)
{
    struct meta_context *op_ctx;
    int ret;

    op_ctx = (struct meta_context *)ctxp;

    if(op_ctx->op != NULL &&
       op_ctx->op->proc.end_scroll != NULL) {
        ret = op_ctx->op->proc.end_scroll(op_ctx->op_context);
    } else {
        ret = 1;
    }

    if(op_ctx->op_context != NULL)
        free(op_ctx->op_context);

    op_ctx->op_context = NULL;

    return ret;
}


static
int MP_OP_API or_init_ctx(void *ctxp, int size, const op_arg_t *arg)
{
    struct meta_context *op_ctx;
    const s_exp_data_t *p;
    wchar_t *name;
    int i;

    if(size != sizeof(struct meta_context)) {
        return 0;
    }

    op_ctx = (struct meta_context *)ctxp;

    /* search and apply operator initializer */
    S_EXP_FOR_EACH(arg->arg, p) {
        op_arg_t op_arg;
        struct scroll_operator_conf *op;

        if(S_EXP_CAR(p)->type != S_EXP_TYPE_CONS ||
           S_EXP_CAAR(p)->type != S_EXP_TYPE_SYMBOL) {
            log_printf(LOG_LEVEL_WARNING,
                       L"scroll operator: invalid format: ");
            log_print_s_exp(LOG_LEVEL_WARNING, S_EXP_CAR(p), 1);
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
            log_printf(LOG_LEVEL_WARNING,
                       L"scroll operator: not found: ");
            log_print_s_exp(LOG_LEVEL_WARNING, S_EXP_CAR(p), 1);
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
                log_printf(LOG_LEVEL_DEBUG,
                           L"scroll operator: "
                           L"start fail (get_context_size): ");
                log_print_s_exp(LOG_LEVEL_DEBUG, S_EXP_CAR(p), 1);
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

                log_printf(LOG_LEVEL_DEBUG,
                           L"scroll operator: "
                           L"start fail (init_context): ");
                log_print_s_exp(LOG_LEVEL_DEBUG, S_EXP_CAR(p), 1);
                continue;
            }
        }

        op_ctx->op = op;

        log_printf(LOG_LEVEL_DEBUG, L"scroll operator: started: ");
        log_print_s_exp(LOG_LEVEL_DEBUG, S_EXP_CAR(p), 1);

        return 1;
    }

    return 0;
}

int MP_OP_API or_get_operator(scroll_op_procs_t *op, int size,
                              const support_procs_t *sprocs)
{
    spr = sprocs;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = meta_get_ctx_size;
    op->init_context = or_init_ctx;
    op->scroll = meta_scroll;
    op->end_scroll = meta_end_scroll;

    return 1;
}


static
int MP_OP_API apply_parent_init_ctx(void *ctxp, int size, const op_arg_t *arg)
{
    struct meta_context *op_ctx;
    const s_exp_data_t *o;
    op_arg_t op_arg;
    struct scroll_operator_conf *op;
    wchar_t *name;
    HWND target;
    int parent_num, i;

    if(size != sizeof(struct meta_context)) {
        return 0;
    }

    op_ctx = (struct meta_context *)ctxp;

    o = S_EXP_CAR(arg->arg);

    if(o->type != S_EXP_TYPE_CONS ||
       S_EXP_CAR(o)->type != S_EXP_TYPE_SYMBOL) {
        log_printf(LOG_LEVEL_WARNING,
                   L"scroll operator: invalid format: ");
        log_print_s_exp(LOG_LEVEL_WARNING, o, 1);
        return 0;
    }

    /* operator name */
    name = S_EXP_CAR(o)->symbol.name;

    /* search operator */
    op = NULL;
    for(i = 0; i < ctx.app_conf.scroll_operator_num; i++) {
        if(wcscmp(name, ctx.app_conf.scroll_operator_conf[i].name) == 0) {
            op = &ctx.app_conf.scroll_operator_conf[i];
            break;
        }
    }
    if(op == NULL) {
        log_printf(LOG_LEVEL_WARNING,
                   L"scroll operator: not found: ");
        log_print_s_exp(LOG_LEVEL_WARNING, o, 1);
        return 0;
    }

    /* parent window */
    {
        s_exp_data_t *n = s_exp_nth(arg->arg, 1);

        if(n != NULL && n->type == S_EXP_TYPE_INTEGER) {
            parent_num = n->number.val;
        } else {
            parent_num = 1;
        }
    }

    target = arg->hwnd;
    for(i = 0; i < parent_num; i++) {
        HWND parent = GetParent(target);
        if(parent == NULL) {
            break;
        }

        target = parent;
    }

    log_printf(LOG_LEVEL_DEBUG, L"apply-parent: %d, %p\n", parent_num, target);

    /* arg for operator */
    memset(&op_arg, 0, sizeof(op_arg));
    op_arg.conf = op->conf;
    op_arg.arg = S_EXP_CDR(o);
    op_arg.hwnd = target;
    op_arg.pos = arg->pos;

    /* get size of operator context */
    if(op->proc.get_context_size != NULL) {
        op_ctx->op_context_size = op->proc.get_context_size(&op_arg);
        if(op_ctx->op_context_size < 0) {
            log_printf(LOG_LEVEL_DEBUG,
                       L"scroll operator: "
                       L"start fail (get_context_size): ");
            log_print_s_exp(LOG_LEVEL_DEBUG, o, 1);
            return 0;
        }
    } else {
        op_ctx->op_context_size = 0;
    }

    /* allocate */
    op_ctx->op_context = malloc(op_ctx->op_context_size);
    if(op_ctx->op_context == NULL) {
        return 0;
    }

    memset(op_ctx->op_context, 0, sizeof(op_ctx->op_context_size));

    /* initialize operator context */
    if(op->proc.init_context != NULL) {
        if(op->proc.init_context(op_ctx->op_context,
                                 op_ctx->op_context_size,
                                 &op_arg) == 0) {
            free(op_ctx->op_context);
            op_ctx->op_context = NULL;

            log_printf(LOG_LEVEL_DEBUG,
                       L"scroll operator: "
                       L"start fail (init_context): ");
            log_print_s_exp(LOG_LEVEL_DEBUG, o, 1);
            return 0;
        }
    }

    op_ctx->op = op;

    log_printf(LOG_LEVEL_DEBUG, L"scroll operator: started: ");
    log_print_s_exp(LOG_LEVEL_DEBUG, o, 1);

    return 1;
}

int MP_OP_API apply_parent_get_operator(scroll_op_procs_t *op, int size,
                                        const support_procs_t *sprocs)
{
    spr = sprocs;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = meta_get_ctx_size;
    op->init_context = apply_parent_init_ctx;
    op->scroll = meta_scroll;
    op->end_scroll = meta_end_scroll;

    return 1;
}
