/*
 * scroll_op.c  -- scroll operators
 *
 * $Id: scroll_op.c,v 1.1 2005/01/12 09:39:47 hos Exp $
 *
 */

#include "scroll_op.h"


static
int get_drag_scroll_delta(int length,
                          int min, int max, double page_ratio,
                          double delta, double *rest)
{
    int range;
    int ret;
    double spd;

    range = max - min + 1;
    spd = (page_ratio >= 1 ? 0 : (range / (length * (1 - page_ratio))));

    ret = delta * spd;
    *rest = delta - (int)ret / spd;
    return ret;
}


static
int scrollbar_line_scroll(HWND hwnd, int bar,
                          HWND msg_hwnd, UINT msg,
                          double *delta, int length)
{
    int i, d;
    WPARAM wp;
    LPARAM lp;

    d = (*delta < 0 ? -(*delta) : *delta);
    wp = MAKEWPARAM((*delta < 0 ? SB_LINELEFT : SB_LINERIGHT), 0);
    lp = (bar == SB_CTL ? (LPARAM)hwnd : 0);

    for(i = 0; i < d; i++) {
        PostMessage(msg_hwnd, msg, wp, lp);
    }

    *delta -= (int)*delta;

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

    d = (*delta < 0 ? -(*delta) : *delta);
    wp = MAKEWPARAM((*delta < 0 ? SB_PAGELEFT : SB_PAGERIGHT), 0);
    lp = (bar == SB_CTL ? (LPARAM)hwnd : 0);

    for(i = 0; i < d; i++) {
        PostMessage(msg_hwnd, msg, wp, lp);
    }

    *delta -= (int)*delta;

    return 1;
}

static
int scrollbar_drag_scroll(HWND hwnd, int bar,
                          HWND msg_hwnd, UINT msg,
                          double *delta, int length)
{
    SCROLLINFO si;
    int dd, pos, min, max;
    double rest;

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    if(GetScrollInfo(hwnd, bar, &si) == 0) {
        return 0;
    }

    min = si.nMin;
    max = si.nMax - (si.nPage ? si.nPage - 1 : 0);
    dd = get_drag_scroll_delta(length, min, max,
                               (double)si.nPage / (si.nMax - si.nMin + 1),
                               *delta, &rest);
    if(dd == 0) {
        return 1;
    }

    pos = si.nPos + dd;
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

    *delta = rest;

    return 1;
}

static
int scrollbar_parcentage_scroll(HWND hwnd, int bar,
                                HWND msg_hwnd, UINT msg,
                                double *delta, int length)
{
}

static
int scrollbar_unit_scroll(HWND hwnd, int bar,
                          HWND msg_hwnd, UINT msg,
                          double *delta, int length)
{
}


enum {
    SCROLLBAR_MODE_LINE = 0,
    SCROLLBAR_MODE_PAGE,
    SCROLLBAR_MODE_DRAG,
    SCROLLBAR_MODE_PERCENTAGE,
    SCROLLBAR_MODE_BARUNIT
};

static
wchar_t *get_mode_name(const s_exp_data_t *arg, const s_exp_data_t *conf,
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

static
int get_scrollbar_mode(const wchar_t *mode_name)
{
    int i, mode;

    static struct {
        wchar_t *name;
        int mode;
    } mode_map[] = {
        {L"line-scroll", SCROLLBAR_MODE_LINE},
        {L"page-scroll", SCROLLBAR_MODE_PAGE},
        {L"drag", SCROLLBAR_MODE_DRAG},
        {L"percentage", SCROLLBAR_MODE_PERCENTAGE},
        {L"bar-unit", SCROLLBAR_MODE_BARUNIT},

        {NULL, 0}
    };

    mode = -1;
    for(i = 0; mode_map[i].name != NULL; i++) {
        if(wcscmp(mode_name, mode_map[i].name) == 0) {
            mode = mode_map[i].mode;
            break;
        }
    }

    return mode;
}

static
void get_scrollbar_ratio(const s_exp_data_t *arg,
                         const s_exp_data_t *mode_conf,
                         double *x_ratio, double *y_ratio,
                         double def_x_ratio, double def_y_ratio)
{
    s_exp_data_t *xr, *yr;

    if((xr = s_exp_nth(arg, 1)) != NULL ||
       (xr = s_exp_nth(mode_conf, 0)) != NULL) {
        if(xr->type == S_EXP_TYPE_INTEGER) {
            *x_ratio = xr->number.val;
        } else if(xr->type == S_EXP_TYPE_FLONUM) {
            *x_ratio = xr->flonum.val;
        } else {
            *x_ratio = def_x_ratio;
        }
    }

    if((yr = s_exp_nth(arg, 2)) != NULL ||
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


struct window_scrollbar_context {
    int mode;

    HWND target;
    SIZE target_size;

    double x_ratio, y_ratio;
    double dx, dy;
};


static
int SCROLL_OP_API window_scrollbar_get_ctx_size(const scroll_op_arg_t *arg)
{
    return sizeof(struct window_scrollbar_context);
}

static
int SCROLL_OP_API window_scrollbar_init_ctx(void *ctxp, int size,
                                            const scroll_op_arg_t *arg)
{
    struct window_scrollbar_context *ctx;
    s_exp_data_t *mode_conf;
    wchar_t *mode_name;

    ctx = (struct window_scrollbar_context *)ctxp;

    /* target window handle */
    ctx->target = arg->hwnd;

    /* scroll mode name */
    mode_name = get_mode_name(arg->arg, arg->conf, L"drag");

    /* scroll mode */
    ctx->mode = get_scrollbar_mode(mode_name);
    if(ctx->mode < 0) {
        return 0;
    }

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

    /* x and y ratio */
    mode_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS, mode_name, NULL);
    if(mode_conf == NULL) {
        mode_conf = S_EXP_NIL;
    }

    get_scrollbar_ratio(arg->arg, mode_conf,
                        &ctx->x_ratio, &ctx->y_ratio, 1.0, 1.0);

    /* window size */
    {
        RECT rt;

        memset(&rt, 0, sizeof(rt));
        GetClientRect(ctx->target, &rt);

        ctx->target_size.cx = rt.right - rt.left + 1;
        ctx->target_size.cy = rt.bottom - rt.top + 1;
    }

    /* initial delta */
    ctx->dx = 0;
    ctx->dy = 0;

    return 1;
}

static
int SCROLL_OP_API window_scrollbar_scroll(void *ctxp, double dx, double dy)
{
    struct window_scrollbar_context *ctx;

    ctx = (struct window_scrollbar_context *)ctxp;

    ctx->dx += dx * ctx->x_ratio;
    ctx->dy += dy * ctx->y_ratio;

    scrollbar_drag_scroll(ctx->target, SB_HORZ, ctx->target, WM_HSCROLL,
                          &ctx->dx, ctx->target_size.cx);
    scrollbar_drag_scroll(ctx->target, SB_VERT, ctx->target, WM_VSCROLL,
                          &ctx->dy, ctx->target_size.cx);

    return 1;
}

static
int SCROLL_OP_API window_scrollbar_end_scroll(void *ctxp)
{
    return 1;
}

int SCROLL_OP_API window_scrollbar_get_operator(scroll_op_procs_t *op,
                                                int api_ver)
{
    if(api_ver < SCROLL_OP_API_VERSION) {
        return 0;
    }

    op->get_context_size = window_scrollbar_get_ctx_size;
    op->init_context = window_scrollbar_init_ctx;
    op->scroll = window_scrollbar_scroll;
    op->end_scroll = window_scrollbar_end_scroll;

    return 1;
}


