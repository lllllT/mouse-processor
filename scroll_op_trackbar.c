/*
 * scroll_op_trackbar.c  -- scroll operator for trackbar control
 *
 * $Id: scroll_op_trackbar.c,v 1.4 2005/01/21 08:54:54 hos Exp $
 *
 */

#include "operator.h"
#include "scroll_op_utils.h"
#include "util.h"
#include <commctrl.h>
#include <math.h>


static const support_procs_t *spr = NULL;


typedef int (* trackbar_scroll_proc_t)(HWND, HWND, UINT, double *, int);

static
int trackbar_line_scroll(HWND hwnd,
                         HWND msg_hwnd, UINT msg,
                         double *delta, int length)
{
    long min, max, line, pos, dd;

    if(SendMessageTimeout(hwnd, TBM_GETRANGEMIN, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &min) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETRANGEMAX, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &max) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETLINESIZE, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &line) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETPOS, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &pos) == 0) {
        return 0;
    }

    dd = trunc(*delta) * line;

    if(dd == 0) {
        return 1;
    }

    pos += dd;
    if(pos < min) {
        pos = min;
    } else if(pos > max) {
        pos = max;
    }

    SendMessageTimeout(hwnd, TBM_SETPOS, TRUE, pos,
                       SMTO_ABORTIFHUNG, 500, NULL);
    SendMessageTimeout(msg_hwnd, msg,
                       MAKEWPARAM((dd > 0 ? TB_LINEDOWN : TB_LINEUP), 0),
                       (LPARAM)hwnd,
                       SMTO_ABORTIFHUNG, 500, NULL);

    *delta -= (double)dd / line;

    return 1;
}

static
int trackbar_page_scroll(HWND hwnd,
                         HWND msg_hwnd, UINT msg,
                         double *delta, int length)
{
    long min, max, page, pos, dd;

    if(SendMessageTimeout(hwnd, TBM_GETRANGEMIN, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &min) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETRANGEMAX, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &max) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETPAGESIZE, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &page) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETPOS, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &pos) == 0) {
        return 0;
    }

    dd = trunc(*delta) * page;

    if(dd == 0) {
        return 1;
    }

    pos += dd;
    if(pos < min) {
        pos = min;
    } else if(pos > max) {
        pos = max;
    }

    SendMessageTimeout(hwnd, TBM_SETPOS, TRUE, pos,
                       SMTO_ABORTIFHUNG, 500, NULL);
    SendMessageTimeout(msg_hwnd, msg,
                       MAKEWPARAM((dd > 0 ? TB_PAGEDOWN : TB_PAGEUP), 0),
                       (LPARAM)hwnd,
                       SMTO_ABORTIFHUNG, 500, NULL);

    *delta -= (double)dd / page;

    return 1;
}

static
int trackbar_r_scroll(HWND hwnd,
                      HWND msg_hwnd, UINT msg,
                      double *delta, int length,
                      int mode)
{
    long min, max, pos, dd;
    double rest;

    if(SendMessageTimeout(hwnd, TBM_GETRANGEMIN, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &min) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETRANGEMAX, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &max) == 0 ||
       SendMessageTimeout(hwnd, TBM_GETPOS, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &pos) == 0) {
        return 0;
    }

    if(mode == 0) {
        dd = get_drag_scroll_delta(length, min, max, 0, *delta, &rest);
    } else if(mode == 1) {
        dd = trunc((*delta) / 100.0 * (max - min));
    } else {
        dd = trunc(*delta);
    }

    if(dd == 0) {
        return 1;
    }

    pos += dd;
    if(pos < min) {
        pos = min;
    } else if(pos > max) {
        pos = max;
    }

    SendMessageTimeout(hwnd, TBM_SETPOS, TRUE, pos,
                       SMTO_ABORTIFHUNG, 500, NULL);
    SendMessageTimeout(msg_hwnd, msg,
                       MAKEWPARAM(TB_THUMBPOSITION, pos),
                       (LPARAM)hwnd,
                       SMTO_ABORTIFHUNG, 500, NULL);

    if(mode == 0) {
        *delta = rest;
    } else if(mode == 1) {
        *delta -= dd * 100.0 / (max - min);
    } else {
        *delta -= dd;
    }

    return 1;
}

static
int trackbar_drag_scroll(HWND hwnd,
                         HWND msg_hwnd, UINT msg,
                         double *delta, int length)
{
    return trackbar_r_scroll(hwnd, msg_hwnd, msg, delta, length, 0);
}

static
int trackbar_percentage_scroll(HWND hwnd,
                               HWND msg_hwnd, UINT msg,
                               double *delta, int length)
{
    return trackbar_r_scroll(hwnd, msg_hwnd, msg, delta, length, 1);
}

static
int trackbar_unit_scroll(HWND hwnd,
                         HWND msg_hwnd, UINT msg,
                         double *delta, int length)
{
    return trackbar_r_scroll(hwnd, msg_hwnd, msg, delta, length, 2);
}


enum {
    TRACKBAR_MODE_LINE = 0,
    TRACKBAR_MODE_PAGE,
    TRACKBAR_MODE_DRAG,
    TRACKBAR_MODE_PERCENTAGE,
    TRACKBAR_MODE_BARUNIT
};

static struct scroll_op_mode_pair trackbar_mode_map[] = {
    {L"line-scroll", TRACKBAR_MODE_LINE},
    {L"page-scroll", TRACKBAR_MODE_PAGE},
    {L"drag", TRACKBAR_MODE_DRAG},
    {L"percentage", TRACKBAR_MODE_PERCENTAGE},
    {L"bar-unit", TRACKBAR_MODE_BARUNIT},

    {NULL, 0}
};

static struct uint_ptr_pair trackbar_scroll_proc_map[] = {
    {TRACKBAR_MODE_LINE, trackbar_line_scroll},
    {TRACKBAR_MODE_PAGE, trackbar_page_scroll},
    {TRACKBAR_MODE_DRAG, trackbar_drag_scroll},
    {TRACKBAR_MODE_PERCENTAGE, trackbar_percentage_scroll},
    {TRACKBAR_MODE_BARUNIT, trackbar_unit_scroll},

    {0, NULL}
};


struct trackbar_control_context {
    int mode;
    trackbar_scroll_proc_t scroll_proc;

    HWND target;
    DWORD target_size;
    int dir;

    HWND parent;

    double x_ratio, y_ratio;
    double ds;
};

static
int MP_OP_API trackbar_control_get_ctx_size(const op_arg_t *arg)
{
    return sizeof(struct trackbar_control_context);
}

static
int MP_OP_API trackbar_control_init_ctx(void *ctxp, int size,
                                        const op_arg_t *arg)
{
    struct trackbar_control_context *ctx;
    s_exp_data_t *mode_conf;
    wchar_t *mode_name;

    if(size != sizeof(struct trackbar_control_context)) {
        return 0;
    }

    ctx = (struct trackbar_control_context *)ctxp;

    /* target window handle */
    ctx->target = arg->hwnd;

    /* scroll mode name */
    mode_name = get_scroll_op_mode_name(arg->arg, arg->conf, L"drag");

    /* scroll mode */
    ctx->mode = get_scroll_op_mode(trackbar_mode_map, mode_name);
    if(ctx->mode < 0) {
        return 0;
    }

    ctx->scroll_proc = (trackbar_scroll_proc_t)
                       assq_pair(trackbar_scroll_proc_map, ctx->mode, NULL);

    {
        LONG_PTR style;

        /* check window style */
        style = GetWindowLongPtr(ctx->target, GWL_STYLE);
        if((style & (TBS_HORZ | TBS_VERT)) != TBS_HORZ &&
           (style & (TBS_HORZ | TBS_VERT)) != TBS_VERT) {
            return 0;
        }

        if((style & (TBS_HORZ | TBS_VERT)) == TBS_HORZ) {
            ctx->dir = TBS_HORZ;
        }
        if((style & (TBS_HORZ | TBS_VERT)) == TBS_VERT) {
            ctx->dir = TBS_VERT;
        }
    }

    /* x and y ratio */
    mode_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS,
                            (ctx->dir == TBS_HORZ ? L"h-bar" : L"v-bar"),
                            mode_name, NULL);
    if(mode_conf == NULL) {
        mode_conf = S_EXP_NIL;
    }

    get_scroll_op_xy_ratio(s_exp_nth_cdr(arg->arg, 1), mode_conf,
                           &ctx->x_ratio, &ctx->y_ratio,
                           (ctx->dir == TBS_HORZ ? 1.0 : 0.0),
                           (ctx->dir == TBS_HORZ ? 0.0 : 1.0));

    /* window size */
    if(SendMessageTimeout(ctx->target, TBM_GETTHUMBLENGTH, 0, 0,
                          SMTO_ABORTIFHUNG, 500, &ctx->target_size) == 0) {
        return 0;
    }

    /* parent window */
    ctx->parent = GetParent(ctx->target);
    if(ctx->parent == NULL) {
        return 0;
    }

    /* initial delta */
    ctx->ds = 0;

    return 1;
}

static
int MP_OP_API trackbar_control_scroll(void *ctxp, double dx, double dy)
{
    struct trackbar_control_context *ctx;

    ctx = (struct trackbar_control_context *)ctxp;

    ctx->ds += dx * ctx->x_ratio +
               dy * ctx->y_ratio;

    ctx->scroll_proc(ctx->target, ctx->parent,
                     (ctx->dir == TBS_HORZ ? WM_HSCROLL : WM_VSCROLL),
                     &ctx->ds, ctx->target_size);

    return 1;
}

static
int MP_OP_API trackbar_control_end_scroll(void *ctxp)
{
    return 1;
}

int MP_OP_API trackbar_control_get_operator(scroll_op_procs_t *op, int size,
                                            const support_procs_t *sprocs)
{
    spr = sprocs;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = trackbar_control_get_ctx_size;
    op->init_context = trackbar_control_init_ctx;
    op->scroll = trackbar_control_scroll;
    op->end_scroll = trackbar_control_end_scroll;

    return 1;
}
