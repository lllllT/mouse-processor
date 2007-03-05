/*
 * scroll_op_wheel.c  -- scroll operator for wheel message
 *
 * $Id: scroll_op_wheel.c,v 1.9 2007/03/05 08:30:41 hos Exp $
 *
 */

#include "operator.h"
#include "scroll_op_utils.h"
#include <math.h>

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020e
#endif


static const support_procs_t *spr = NULL;


struct wheel_message_context {
    HWND target;
    POINT pt;

    double x_ratio, y_ratio;
    int tick;
    double dx, dy;
};

static
int MP_OP_API wheel_message_get_ctx_size(const op_arg_t *arg)
{
    return sizeof(struct wheel_message_context);
}

static
int MP_OP_API wheel_message_init_ctx(void *ctxp, int size, const op_arg_t *arg)
{
    struct wheel_message_context *ctx;
    s_exp_data_t *ratio_conf, *data;

    if(size != sizeof(struct wheel_message_context)) {
        return 0;
    }

    ctx = (struct wheel_message_context *)ctxp;

    /* target window handle */
    ctx->target = arg->hwnd;

    /* x and y ratio */
    ratio_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS, L"ratio", NULL);
    if(ratio_conf == NULL) {
        ratio_conf = S_EXP_NIL;
    }

    get_scroll_op_xy_ratio(arg->arg, ratio_conf,
                           &ctx->x_ratio, &ctx->y_ratio,
                           5.0, -5.0);

    /* tick */
    if(((data = s_exp_nth(arg->arg, 2)) != NULL &&
        (data->type == S_EXP_TYPE_INTEGER ||
         data->type == S_EXP_TYPE_FLONUM)) ||
       ((data = s_exp_assq(arg->conf, L"tick")) != NULL &&
        (data->type == S_EXP_TYPE_INTEGER ||
         data->type == S_EXP_TYPE_FLONUM))) {
        if(data->type == S_EXP_TYPE_INTEGER) {
            ctx->tick = data->number.val;
        } else {
            ctx->tick = data->flonum.val;
        }
    } else {
        ctx->tick = 120;
    }

    /* cursor position */
    ctx->pt = arg->pos;

    /* initial delta */
    ctx->dx = 0;
    ctx->dy = 0;

    return 1;
}

static
int MP_OP_API wheel_message_scroll(void *ctxp, double dx, double dy)
{
    struct wheel_message_context *ctx;
    int nx, ny, data;
    WORD wlow;
    WPARAM wp;
    LPARAM lp;

    ctx = (struct wheel_message_context *)ctxp;

    ctx->dx += dx * ctx->x_ratio;
    ctx->dy += dy * ctx->y_ratio;

    nx = (int)trunc(ctx->dx / ctx->tick) * ctx->tick;
    ny = (int)trunc(ctx->dy / ctx->tick) * ctx->tick;

    wlow = (GetKeyState(VK_CONTROL)  & 0x8000 ? MK_CONTROL  : 0) |
           (GetKeyState(VK_LBUTTON)  & 0x8000 ? MK_LBUTTON  : 0) |
           (GetKeyState(VK_MBUTTON)  & 0x8000 ? MK_MBUTTON  : 0) |
           (GetKeyState(VK_RBUTTON)  & 0x8000 ? MK_RBUTTON  : 0) |
           (GetKeyState(VK_SHIFT)    & 0x8000 ? MK_SHIFT    : 0) |
           (GetKeyState(VK_XBUTTON1) & 0x8000 ? MK_XBUTTON1 : 0) |
           (GetKeyState(VK_XBUTTON2) & 0x8000 ? MK_XBUTTON2 : 0);
    lp = MAKELPARAM(ctx->pt.x, ctx->pt.y);

    if(ny != 0) {
        wp = MAKEWPARAM(wlow, ny);
        PostMessage(ctx->target, WM_MOUSEWHEEL, wp, lp);
    }
    if(nx != 0) {
        wp = MAKEWPARAM(wlow, nx);
        PostMessage(ctx->target, WM_MOUSEHWHEEL, wp, lp);
    }

    ctx->dx -= nx;
    ctx->dy -= ny;

    return 1;
}

static
int MP_OP_API wheel_message_end_scroll(void *ctxp)
{
    return 1;
}

int MP_OP_API wheel_message_get_operator(scroll_op_procs_t *op, int size,
                                         const support_procs_t *sprocs)
{
    spr = sprocs;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = wheel_message_get_ctx_size;
    op->init_context = wheel_message_init_ctx;
    op->scroll = wheel_message_scroll;
    op->end_scroll = wheel_message_end_scroll;

    return 1;
}
