/*
 * scroll_op_tab.c  -- scroll operator for tab control
 *
 * $Id: scroll_op_tab.c,v 1.2 2005/01/14 19:13:59 hos Exp $
 *
 */

#define _WIN32_IE 0x0300

#include "scroll_op.h"
#include "scroll_op_utils.h"
#include <commctrl.h>
#include <math.h>


struct tab_control_context {
    HWND target;
    int ctrl_id;

    HWND parent;

    double x_ratio, y_ratio;
    int tick;
    double ds;
};

static
int SCROLL_OP_API tab_control_get_ctx_size(const scroll_op_arg_t *arg)
{
    return sizeof(struct tab_control_context);
}

static
int SCROLL_OP_API tab_control_init_ctx(void *ctxp, int size,
                                         const scroll_op_arg_t *arg)
{
    struct tab_control_context *ctx;
    s_exp_data_t *ratio_conf, *data;
    LONG_PTR style;

    if(size != sizeof(struct tab_control_context)) {
        return 0;
    }

    ctx = (struct tab_control_context *)ctxp;

    /* target window handle */
    ctx->target = arg->hwnd;

    /* window style */
    style = GetWindowLongPtr(ctx->target, GWL_STYLE);

    /* x and y ratio */
    ratio_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS,
                             (style & TCS_VERTICAL ? L"v-tab" : L"h-tab"),
                             L"ratio", NULL);
    if(ratio_conf == NULL) {
        ratio_conf = S_EXP_NIL;
    }

    get_scroll_op_xy_ratio(arg->arg, ratio_conf,
                           &ctx->x_ratio, &ctx->y_ratio,
                           (style & TCS_VERTICAL ? 0.0 : 5.0),
                           (style & TCS_VERTICAL ? 5.0 : 0.0));

    /* tick */
    if(((data = s_exp_nth(arg->arg, 2)) != NULL &&
        (data->type == S_EXP_TYPE_INTEGER ||
         data->type == S_EXP_TYPE_FLONUM)) ||
       ((data = s_exp_massq(arg->conf, S_EXP_TYPE_INTEGER,
                            (style & TCS_VERTICAL ? L"v-tab" : L"h-tab"),
                            L"tick", NULL)) != NULL ||
        (data = s_exp_massq(arg->conf, S_EXP_TYPE_FLONUM,
                            (style & TCS_VERTICAL ? L"v-tab" : L"h-tab"),
                            L"tick", NULL)) != NULL)) {
        if(data->type == S_EXP_TYPE_INTEGER) {
            ctx->tick = data->number.val;
        } else {
            ctx->tick = data->flonum.val;
        }
    } else {
        ctx->tick = 120;
    }

    /* parent window */
    ctx->parent = GetParent(ctx->target);
    if(ctx->parent == NULL) {
        return 0;
    }

    /* control ID */
    ctx->ctrl_id = GetDlgCtrlID(ctx->target);
    if(ctx->ctrl_id == 0) {
        return 0;
    }

    /* initial delta */
    ctx->ds = 0;

    return 1;
}

static
int SCROLL_OP_API tab_control_scroll(void *ctxp, double dx, double dy)
{
    struct tab_control_context *ctx;
    long pos, nitem;
    NMHDR nmh;
    int d;

    ctx = (struct tab_control_context *)ctxp;

    ctx->ds += dx * ctx->x_ratio +
               dy * ctx->y_ratio;

    if(SendMessageTimeout(ctx->target, TCM_GETCURSEL, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &pos) == 0 ||
       SendMessageTimeout(ctx->target, TCM_GETITEMCOUNT, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &nitem) == 0) {
        return 0;
    }

    if(nitem <= 0) {
        return 0;
    }

    d = trunc(ctx->ds / ctx->tick);
    if(d == 0) {
        return 1;
    }

    pos = (pos < 0 ? d : pos + d);
    if(pos < 0) {
        pos = 0;
    } else if(pos > nitem) {
        pos = nitem;
    }

    memset(&nmh, 0, sizeof(nmh));
    nmh.hwndFrom = ctx->target;
    nmh.idFrom = ctx->ctrl_id;
    nmh.code = TCN_SELCHANGING;
    SendMessageTimeout(ctx->parent, WM_NOTIFY, ctx->ctrl_id, (LPARAM)&nmh,
                       SMTO_ABORTIFHUNG, 500, NULL);

    SendMessageTimeout(ctx->target, TCM_SETCURSEL, pos, 0,
                       SMTO_ABORTIFHUNG, 500, NULL);

    nmh.code = TCN_SELCHANGE;
    SendMessageTimeout(ctx->parent, WM_NOTIFY, ctx->ctrl_id, (LPARAM)&nmh,
                       SMTO_ABORTIFHUNG, 500, NULL);

    ctx->ds -= d * ctx->tick;

    return 1;
}

static
int SCROLL_OP_API tab_control_end_scroll(void *ctxp)
{
    return 1;
}

int SCROLL_OP_API tab_control_get_operator(scroll_op_procs_t *op,
                                                int api_ver)
{
    if(api_ver < SCROLL_OP_API_VERSION) {
        return 0;
    }

    op->get_context_size = tab_control_get_ctx_size;
    op->init_context = tab_control_init_ctx;
    op->scroll = tab_control_scroll;
    op->end_scroll = tab_control_end_scroll;

    return 1;
}
