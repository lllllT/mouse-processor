/*
 * scroll_op_scrollbar.c  -- scroll operators for scrollbar
 *
 * $Id: scroll_op_scrollbar.c,v 1.9 2005/01/24 05:06:14 hos Exp $
 *
 */

#include "operator.h"
#include "scroll_op_utils.h"
#include "util.h"
#include <tchar.h>
#include <commctrl.h>
#include <math.h>


static const support_procs_t *spr = NULL;


#define SCROLLBAR_STYLE_MASK (SBS_HORZ | SBS_VERT | SBS_SIZEBOX | SBS_SIZEGRIP)


typedef int (* scrollbar_scroll_proc_t)(HWND, int, HWND, UINT, double *, int);

static
int scrollbar_line_scroll(HWND hwnd, int bar,
                          HWND msg_hwnd, UINT msg,
                          double *delta, int length)
{
    int i, d;
    WPARAM wp;
    LPARAM lp;

    d = trunc(*delta < 0 ? -(*delta) : *delta);
    wp = MAKEWPARAM((*delta < 0 ? SB_LINELEFT : SB_LINERIGHT), 0);
    lp = (bar == SB_CTL ? (LPARAM)hwnd : 0);

    for(i = 0; i < d; i++) {
        PostMessage(msg_hwnd, msg, wp, lp);
    }

    *delta -= trunc(*delta);

    return 1;
}

static
int scrollbar_page_scroll(HWND hwnd, int bar,
                          HWND msg_hwnd, UINT msg,
                          double *delta, int length)
{
    int i, d;
    WPARAM wp;
    LPARAM lp;

    d = trunc(*delta < 0 ? -(*delta) : *delta);
    wp = MAKEWPARAM((*delta < 0 ? SB_PAGELEFT : SB_PAGERIGHT), 0);
    lp = (bar == SB_CTL ? (LPARAM)hwnd : 0);

    for(i = 0; i < d; i++) {
        PostMessage(msg_hwnd, msg, wp, lp);
    }

    *delta -= trunc(*delta);

    return 1;
}

static
int scrollbar_r_scroll(HWND hwnd, int bar,
                       HWND msg_hwnd, UINT msg,
                       double *delta, int length,
                       int mode)
{
    SCROLLINFO si;
    int dd, pos, org_pos, min, max;
    double rest;

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    if(GetScrollInfo(hwnd, bar, &si) == 0) {
        return 0;
    }

    min = si.nMin;
    max = si.nMax - (si.nPage ? si.nPage - 1 : 0);
    org_pos = si.nPos;

    if(mode == 0) {             /* drag */
        dd = get_drag_scroll_delta(length, min, max,
                                   (double)si.nPage / (si.nMax - si.nMin + 1),
                                   *delta, &rest);
    } else if(mode == 1) {      /* percentage */
        dd = trunc((*delta) / 100.0 * (max - min));
    } else {                    /* bar-unit */
        dd = trunc(*delta);
    }

    if(dd == 0) {
        return 1;
    }

    pos = org_pos + dd;
    if(pos < min) {
        pos = min;
    } else if(pos > max) {
        pos = max;
    }

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = pos;
    SetScrollInfo(hwnd, bar, &si, FALSE);
    SendMessageTimeout(msg_hwnd, msg,
                       MAKEWPARAM(SB_THUMBPOSITION, pos),
                       (bar == SB_CTL ? (LPARAM)hwnd : 0),
                       SMTO_ABORTIFHUNG, 500, NULL);

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    GetScrollInfo(hwnd, bar, &si);

    if(si.nPos != org_pos || pos == min || pos == max) {
        if(mode == 0) {             /* drag */
            *delta = rest;
        } else if(mode == 1) {      /* percentage */
            *delta -= dd * 100.0 / (max - min);
        } else {                    /* bar-unit */
            *delta -= dd;
        }
    }

    return 1;
}

static
int scrollbar_drag_scroll(HWND hwnd, int bar,
                          HWND msg_hwnd, UINT msg,
                          double *delta, int length)
{
    return scrollbar_r_scroll(hwnd, bar, msg_hwnd, msg, delta, length, 0);
}

static
int scrollbar_percentage_scroll(HWND hwnd, int bar,
                                HWND msg_hwnd, UINT msg,
                                double *delta, int length)
{
    return scrollbar_r_scroll(hwnd, bar, msg_hwnd, msg, delta, length, 1);
}

static
int scrollbar_unit_scroll(HWND hwnd, int bar,
                          HWND msg_hwnd, UINT msg,
                          double *delta, int length)
{
    return scrollbar_r_scroll(hwnd, bar, msg_hwnd, msg, delta, length, 2);
}


enum {
    SCROLLBAR_MODE_LINE = 0,
    SCROLLBAR_MODE_PAGE,
    SCROLLBAR_MODE_DRAG,
    SCROLLBAR_MODE_PERCENTAGE,
    SCROLLBAR_MODE_BARUNIT
};

static struct scroll_op_mode_pair scrollbar_mode_map[] = {
    {L"line-scroll", SCROLLBAR_MODE_LINE},
    {L"page-scroll", SCROLLBAR_MODE_PAGE},
    {L"drag", SCROLLBAR_MODE_DRAG},
    {L"percentage", SCROLLBAR_MODE_PERCENTAGE},
    {L"bar-unit", SCROLLBAR_MODE_BARUNIT},

    {NULL, 0}
};

static struct uint_ptr_pair scrollbar_scroll_proc_map[] = {
    {SCROLLBAR_MODE_LINE, scrollbar_line_scroll},
    {SCROLLBAR_MODE_PAGE, scrollbar_page_scroll},
    {SCROLLBAR_MODE_DRAG, scrollbar_drag_scroll},
    {SCROLLBAR_MODE_PERCENTAGE, scrollbar_percentage_scroll},
    {SCROLLBAR_MODE_BARUNIT, scrollbar_unit_scroll},

    {0, NULL}
};


struct window_scrollbar_context {
    int mode;
    scrollbar_scroll_proc_t scroll_proc;

    HWND target;
    SIZE target_size;

    double x_ratio, y_ratio;
    double dx, dy;
};

static
int MP_OP_API window_scrollbar_get_ctx_size(const op_arg_t *arg)
{
    return sizeof(struct window_scrollbar_context);
}

static
int MP_OP_API window_scrollbar_init_ctx(void *ctxp, int size,
                                        const op_arg_t *arg)
{
    struct window_scrollbar_context *ctx;
    s_exp_data_t *mode_conf;
    wchar_t *mode_name;

    if(size != sizeof(struct window_scrollbar_context)) {
        return 0;
    }

    ctx = (struct window_scrollbar_context *)ctxp;

    /* target window handle */
    ctx->target = arg->hwnd;

    /* scroll mode name */
    mode_name = get_scroll_op_mode_name(arg->arg, arg->conf, L"drag");

    /* scroll mode */
    ctx->mode = get_scroll_op_mode(scrollbar_mode_map, mode_name);
    if(ctx->mode < 0) {
        return 0;
    }

    ctx->scroll_proc = (scrollbar_scroll_proc_t)
                       assq_pair(scrollbar_scroll_proc_map, ctx->mode, NULL);

    /* x and y ratio */
    mode_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS, mode_name, NULL);
    if(mode_conf == NULL) {
        mode_conf = S_EXP_NIL;
    }

    get_scroll_op_xy_ratio(s_exp_nth_cdr(arg->arg, 1), mode_conf,
                           &ctx->x_ratio, &ctx->y_ratio, 1.0, 1.0);

    /* check window style */
    switch(ctx->mode) {
      case SCROLLBAR_MODE_DRAG:
      case SCROLLBAR_MODE_PERCENTAGE:
      case SCROLLBAR_MODE_BARUNIT:
      {
          LONG_PTR style;

          style = GetWindowLongPtr(ctx->target, GWL_STYLE);
          if((style & (WS_HSCROLL | WS_VSCROLL)) == 0) {
              return 0;
          }

          break;
      }
    }

    /* window size */
    {
        RECT rt;

        memset(&rt, 0, sizeof(rt));
        if(GetClientRect(ctx->target, &rt) == 0) {
            return 0;
        }

        ctx->target_size.cx = rt.right - rt.left + 1;
        ctx->target_size.cy = rt.bottom - rt.top + 1;
    }

    /* initial delta */
    ctx->dx = 0;
    ctx->dy = 0;

    return 1;
}

static
int MP_OP_API window_scrollbar_scroll(void *ctxp, double dx, double dy)
{
    struct window_scrollbar_context *ctx;

    ctx = (struct window_scrollbar_context *)ctxp;

    ctx->dx += dx * ctx->x_ratio;
    ctx->dy += dy * ctx->y_ratio;

    ctx->scroll_proc(ctx->target, SB_HORZ, ctx->target, WM_HSCROLL,
                     &ctx->dx, ctx->target_size.cx);
    ctx->scroll_proc(ctx->target, SB_VERT, ctx->target, WM_VSCROLL,
                     &ctx->dy, ctx->target_size.cx);

    return 1;
}

static
int MP_OP_API window_scrollbar_end_scroll(void *ctxp)
{
    return 1;
}

int MP_OP_API window_scrollbar_get_operator(scroll_op_procs_t *op, int size,
                                            const support_procs_t *sprocs)
{
    spr = sprocs;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = window_scrollbar_get_ctx_size;
    op->init_context = window_scrollbar_init_ctx;
    op->scroll = window_scrollbar_scroll;
    op->end_scroll = window_scrollbar_end_scroll;

    return 1;
}


static int nb_x_margin = 0, nb_y_margin = 0;
static int nb_x_sbw = 0, nb_y_sbh = 0;

struct neighborhood_scrollbar_context {
    int mode;
    scrollbar_scroll_proc_t scroll_proc;

    HWND target;
    RECT target_rect;

    HWND h_bar;
    HWND v_bar;
    SIZE target_size;

    HWND h_parent;
    HWND v_parent;

    double x_ratio, y_ratio;
    double dx, dy;
};

static
int MP_OP_API neighborhood_scrollbar_get_ctx_size(const op_arg_t *arg)
{
    return sizeof(struct neighborhood_scrollbar_context);
}

inline static
int is_in_margin(int v1, int v2, int m)
{
    return ((v1 >= v2 - m) && (v1 <= v2 + m));
}

inline static
int is_in_range(int v1, int v2, int min, int max, int m)
{
    return ((v1 >= min - m) && (v2 <= max + m));
}

static
BOOL CALLBACK enum_neighborhood_scrollbar(HWND hwnd, LPARAM lparam)
{
    struct neighborhood_scrollbar_context *ctx;
    TCHAR class[256];
    RECT rt;
    LONG_PTR style;

    ctx = (struct neighborhood_scrollbar_context *)lparam;

    if(GetClassName(hwnd, class, 256) == 0) {
        return TRUE;
    }

    if(_tcscmp(class, _T("ScrollBar")) != 0) {
        return TRUE;
    }

    if(GetWindowRect(hwnd, &rt) == 0) {
        return TRUE;
    }

    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if(style == 0) {
        return TRUE;
    }

    if((style & WS_VISIBLE) == 0) {
        return TRUE;
    }

    if((style & SCROLLBAR_STYLE_MASK) == SBS_HORZ) {
        if((is_in_margin(rt.top, ctx->target_rect.bottom, nb_y_margin) ||
            is_in_margin(rt.top, ctx->target_rect.top, nb_y_margin) ||
            is_in_margin(rt.bottom, ctx->target_rect.top, nb_y_margin) ||
            is_in_margin(rt.bottom, ctx->target_rect.bottom, nb_y_margin)) &&
           is_in_range(rt.left, rt.right,
                       ctx->target_rect.left, ctx->target_rect.right,
                       nb_x_margin + nb_x_sbw)) {
            ctx->h_parent = GetParent(hwnd);
            if(ctx->h_parent == NULL) {
                return TRUE;
            }

            if(ctx->h_bar != NULL) {
                ctx->h_bar = ctx->v_bar = NULL;
                return FALSE;
            }

            ctx->h_bar = hwnd;
            ctx->target_size.cx = rt.right - rt.left;
            return TRUE;
        }
    }

    if((style & SCROLLBAR_STYLE_MASK) == SBS_VERT) {
        if((is_in_margin(rt.left, ctx->target_rect.right, nb_x_margin) ||
            is_in_margin(rt.left, ctx->target_rect.left, nb_x_margin) ||
            is_in_margin(rt.right, ctx->target_rect.left, nb_x_margin) ||
            is_in_margin(rt.right, ctx->target_rect.right, nb_x_margin)) &&
           is_in_range(rt.top, rt.bottom,
                       ctx->target_rect.top, ctx->target_rect.bottom,
                       nb_y_margin + nb_y_sbh)) {
            ctx->v_parent = GetParent(hwnd);
            if(ctx->v_parent == NULL) {
                return TRUE;
            }

            if(ctx->v_bar != NULL) {
                ctx->h_bar = ctx->v_bar = NULL;
                return FALSE;
            }

            ctx->v_bar = hwnd;
            ctx->target_size.cy = rt.bottom - rt.top;
            return TRUE;
        }
    }

    return TRUE;
}

static
int MP_OP_API neighborhood_scrollbar_init_ctx(void *ctxp, int size,
                                              const op_arg_t *arg)
{
    struct neighborhood_scrollbar_context *ctx;
    s_exp_data_t *mode_conf;
    wchar_t *mode_name;

    if(size != sizeof(struct neighborhood_scrollbar_context)) {
        return 0;
    }

    ctx = (struct neighborhood_scrollbar_context *)ctxp;

    /* target window handle */
    ctx->target = arg->hwnd;

    /* scroll mode name */
    mode_name = get_scroll_op_mode_name(arg->arg, arg->conf, L"drag");

    /* scroll mode */
    ctx->mode = get_scroll_op_mode(scrollbar_mode_map, mode_name);
    if(ctx->mode < 0) {
        return 0;
    }

    ctx->scroll_proc = (scrollbar_scroll_proc_t)
                       assq_pair(scrollbar_scroll_proc_map, ctx->mode, NULL);

    /* x and y ratio */
    mode_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS, mode_name, NULL);
    if(mode_conf == NULL) {
        mode_conf = S_EXP_NIL;
    }

    get_scroll_op_xy_ratio(s_exp_nth_cdr(arg->arg, 1), mode_conf,
                           &ctx->x_ratio, &ctx->y_ratio, 1.0, 1.0);

    /* window rect */
    if(GetWindowRect(ctx->target, &ctx->target_rect) == 0) {
        return 0;
    }

    /* enum margin */
    nb_x_margin = GetSystemMetrics(SM_CXSIZEFRAME) + 1;
    nb_y_margin = GetSystemMetrics(SM_CYSIZEFRAME) + 1;
    nb_x_sbw = GetSystemMetrics(SM_CXVSCROLL);
    nb_y_sbh = GetSystemMetrics(SM_CYHSCROLL);

    /* search neighborhood scrollbars */
    ctx->h_bar = ctx->v_bar = NULL;
    EnumChildWindows(ctx->target, enum_neighborhood_scrollbar, (LPARAM)ctx);

    if(ctx->h_bar == NULL && ctx->v_bar == NULL) {
        HWND parent = GetParent(ctx->target);

        if(parent == NULL) {
            return 0;
        }

        ctx->h_bar = ctx->v_bar = NULL;
        EnumChildWindows(parent, enum_neighborhood_scrollbar, (LPARAM)ctx);

        if(ctx->h_bar == NULL && ctx->v_bar == NULL) {
            return 0;
        }
    }

    /* initial delta */
    ctx->dx = 0;
    ctx->dy = 0;

    return 1;
}

static
int MP_OP_API neighborhood_scrollbar_scroll(void *ctxp,
                                            double dx, double dy)
{
    struct neighborhood_scrollbar_context *ctx;

    ctx = (struct neighborhood_scrollbar_context *)ctxp;

    ctx->dx += dx * ctx->x_ratio;
    ctx->dy += dy * ctx->y_ratio;

    if(ctx->h_bar != NULL) {
        ctx->scroll_proc(ctx->h_bar, SB_CTL, ctx->h_parent, WM_HSCROLL,
                         &ctx->dx, ctx->target_size.cx);
    }
    if(ctx->v_bar != NULL) {
        ctx->scroll_proc(ctx->v_bar, SB_CTL, ctx->v_parent, WM_VSCROLL,
                         &ctx->dy, ctx->target_size.cy);
    }

    return 1;
}

static
int MP_OP_API neighborhood_scrollbar_end_scroll(void *ctxp)
{
    return 1;
}

int MP_OP_API neighborhood_scrollbar_get_operator(scroll_op_procs_t *op,
                                                  int size,
                                                  const support_procs_t *sproc)
{
    spr = sproc;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = neighborhood_scrollbar_get_ctx_size;
    op->init_context = neighborhood_scrollbar_init_ctx;
    op->scroll = neighborhood_scrollbar_scroll;
    op->end_scroll = neighborhood_scrollbar_end_scroll;

    return 1;
}


struct scrollbar_control_context {
    int mode;
    scrollbar_scroll_proc_t scroll_proc;

    HWND target;
    int target_size;
    int dir;

    HWND parent;

    double x_ratio, y_ratio;
    double ds;
};

static
int MP_OP_API scrollbar_control_get_ctx_size(const op_arg_t *arg)
{
    return sizeof(struct scrollbar_control_context);
}

static
int MP_OP_API scrollbar_control_init_ctx(void *ctxp, int size,
                                         const op_arg_t *arg)
{
    struct scrollbar_control_context *ctx;
    s_exp_data_t *mode_conf;
    wchar_t *mode_name;

    if(size != sizeof(struct scrollbar_control_context)) {
        return 0;
    }

    ctx = (struct scrollbar_control_context *)ctxp;

    /* target window handle */
    ctx->target = arg->hwnd;

    /* scroll mode name */
    mode_name = get_scroll_op_mode_name(arg->arg, arg->conf, L"drag");

    /* scroll mode */
    ctx->mode = get_scroll_op_mode(scrollbar_mode_map, mode_name);
    if(ctx->mode < 0) {
        return 0;
    }

    ctx->scroll_proc = (scrollbar_scroll_proc_t)
                       assq_pair(scrollbar_scroll_proc_map, ctx->mode, NULL);

    {
        LONG_PTR style;

        /* check window style */
        style = GetWindowLongPtr(ctx->target, GWL_STYLE);
        if((style & SCROLLBAR_STYLE_MASK) != SBS_HORZ &&
           (style & SCROLLBAR_STYLE_MASK) != SBS_VERT) {
            return 0;
        }

        if((style & SCROLLBAR_STYLE_MASK) == SBS_HORZ) {
            ctx->dir = SBS_HORZ;
        }
        if((style & SCROLLBAR_STYLE_MASK) == SBS_VERT) {
            ctx->dir = SBS_VERT;
        }
    }

    /* x and y ratio */
    mode_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS,
                            (ctx->dir == SBS_HORZ ? L"h-bar" : L"v-bar"),
                            mode_name, NULL);
    if(mode_conf == NULL) {
        mode_conf = S_EXP_NIL;
    }

    get_scroll_op_xy_ratio(s_exp_nth_cdr(arg->arg, 1), mode_conf,
                           &ctx->x_ratio, &ctx->y_ratio,
                           (ctx->dir == SBS_HORZ ? 1.0 : 0.0),
                           (ctx->dir == SBS_HORZ ? 0.0 : 1.0));

    /* window size */
    {
        RECT rt;

        memset(&rt, 0, sizeof(rt));
        if(GetClientRect(ctx->target, &rt) == 0) {
            return 0;
        }

        if(ctx->dir == SBS_HORZ) {
            ctx->target_size = rt.right - rt.left + 1;
        } else {
            ctx->target_size = rt.bottom - rt.top + 1;
        }
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
int MP_OP_API scrollbar_control_scroll(void *ctxp, double dx, double dy)
{
    struct scrollbar_control_context *ctx;

    ctx = (struct scrollbar_control_context *)ctxp;

    ctx->ds += dx * ctx->x_ratio +
               dy * ctx->y_ratio;

    ctx->scroll_proc(ctx->target, SB_CTL,
                     ctx->parent,
                     (ctx->dir == SBS_HORZ ? WM_HSCROLL : WM_VSCROLL),
                     &ctx->ds, ctx->target_size);

    return 1;
}

static
int MP_OP_API scrollbar_control_end_scroll(void *ctxp)
{
    return 1;
}

int MP_OP_API scrollbar_control_get_operator(scroll_op_procs_t *op, int size,
                                             const support_procs_t *sprocs)
{
    spr = sprocs;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = scrollbar_control_get_ctx_size;
    op->init_context = scrollbar_control_init_ctx;
    op->scroll = scrollbar_control_scroll;
    op->end_scroll = scrollbar_control_end_scroll;

    return 1;
}
