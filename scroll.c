/*
 * scroll.c  -- scroll window
 *
 * $Id: scroll.c,v 1.5 2005/01/05 09:30:33 hos Exp $
 *
 */

#include "main.h"
#include "util.h"
#include <commctrl.h>


static
int get_scroll_pos(HWND hwnd, int bar, int delta, int length, int *ret_pos)
{
    SCROLLINFO si;
    int min, max, pos;
    double spd;

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    if(GetScrollInfo(hwnd, bar, &si) == 0) {
        *ret_pos = 0;
        return 0;
    }

    {
        int range;
        double page_ratio;

        min = si.nMin;
        max = si.nMax - (si.nPage ? si.nPage - 1 : 0);
        range = max - min + 1;
        page_ratio = (double)si.nPage / (si.nMax - si.nMin + 1);
        spd = (page_ratio == 1 ? 0 : (range / (length * (1 - page_ratio))));
        pos = si.nPos;
    }

    delta *= spd;
    if(delta == 0) {
        *ret_pos = pos;
        return 0;
    }

    pos += delta;
    if(pos < min) {
        pos = min;
    } else if(pos > max) {
        pos = max;
    }

    *ret_pos = pos;
    return 1;
}


static
int scroll_window(HWND hwnd, int bar, int delta, int length)
{
    int pos;
    SCROLLINFO si;

    if(delta == 0) {
        return 0;
    }

    if(! get_scroll_pos(hwnd, bar, delta, length, &pos)) {
        return 0;
    }

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = pos;
    SetScrollInfo(hwnd, bar, &si, FALSE);
    SendMessageTimeout(hwnd,
                       (bar == SB_HORZ ? WM_HSCROLL : WM_VSCROLL),
                       MAKEWPARAM(SB_THUMBPOSITION, pos), 0,
                       SMTO_ABORTIFHUNG, 500, NULL);

    return 1;
}

static
void scroll_line(HWND hwnd, int bar, int delta)
{
    int i, n, dir;

    if(delta == 0) {
        return;
    }

    n = (delta < 0 ? -delta : delta);
    dir = (delta < 0 ? SB_LINELEFT : SB_LINERIGHT);

    for(i = 0; i < n; i++) {
        PostMessage(hwnd,
                    (bar == SB_HORZ ? WM_HSCROLL : WM_VSCROLL),
                    MAKEWPARAM(dir, 0), 0);
    }
}


static
void scrolling_native_h(int x, int y)
{
    if(scroll_window(ctx.scroll_data.target, SB_HORZ,
                     ctx.scroll_data.dx, ctx.scroll_data.target_size.cx)) {
        ctx.scroll_data.dx -= (int)ctx.scroll_data.dx;
    }
}

static
void scrolling_native_v(int x, int y)
{
    if(scroll_window(ctx.scroll_data.target, SB_VERT,
                     ctx.scroll_data.dy, ctx.scroll_data.target_size.cy)) {
        ctx.scroll_data.dy -= (int)ctx.scroll_data.dy;
    }
}

static
void scrolling_native_hv(int x, int y)
{
    scrolling_native_h(x, y);
    scrolling_native_v(x, y);
}

static
void scrolling_line(int x, int y)
{
    int dxi, dyi;

    dxi = ctx.scroll_data.dx * ctx.scroll_line.x_ratio;
    dyi = ctx.scroll_data.dy * ctx.scroll_line.y_ratio;

    scroll_line(ctx.scroll_data.target, SB_HORZ, dxi);
    scroll_line(ctx.scroll_data.target, SB_VERT, dyi);

    ctx.scroll_data.dx -= dxi / ctx.scroll_line.x_ratio;
    ctx.scroll_data.dy -= dyi / ctx.scroll_line.y_ratio;
}

static
void scrolling_ie(int x, int y)
{
    if(scroll_ie_h(ctx.scroll_data.ie_target,
                   ctx.scroll_data.dx, ctx.scroll_data.target_size.cx)) {
        ctx.scroll_data.dx -= (int)ctx.scroll_data.dx;
    }

    if(scroll_ie_v(ctx.scroll_data.ie_target,
                   ctx.scroll_data.dy, ctx.scroll_data.target_size.cy)) {
        ctx.scroll_data.dy -= (int)ctx.scroll_data.dy;
    }
}

static
void scrolling_wheel(int x, int y)
{
    int wn, data, i;

    ctx.scroll_data.dw +=
        ctx.scroll_data.dx * ctx.scroll_wheel.x_ratio +
        ctx.scroll_data.dy * ctx.scroll_wheel.y_ratio;
    ctx.scroll_data.dx = 0;
    ctx.scroll_data.dy = 0;

    wn= ctx.scroll_data.dw / ctx.scroll_wheel.tick;
    if(wn == 0) {
        return;
    }

    data = ctx.scroll_wheel.tick;
    if(wn < 0) {
        wn = -wn;
        data = -data;
    }

    for(i = 0; i < wn; i++) {
        PostMessage(
            ctx.scroll_data.target,
            WM_MOUSEWHEEL,
            MAKEWPARAM(
                (GetAsyncKeyState(VK_CONTROL) & 0x8000 ? MK_CONTROL : 0) |
                (GetAsyncKeyState(VK_LBUTTON) & 0x8000 ? MK_LBUTTON : 0) |
                (GetAsyncKeyState(VK_MBUTTON) & 0x8000 ? MK_MBUTTON : 0) |
                (GetAsyncKeyState(VK_RBUTTON) & 0x8000 ? MK_RBUTTON : 0) |
                (GetAsyncKeyState(VK_SHIFT) & 0x8000 ? MK_SHIFT : 0) |
                (GetAsyncKeyState(VK_XBUTTON1) & 0x8000 ? MK_XBUTTON1 : 0) |
                (GetAsyncKeyState(VK_XBUTTON2) & 0x8000 ? MK_XBUTTON2 : 0),
                data),
            MAKELPARAM(x, y));
    }

    ctx.scroll_data.dw -= data * wn;
}


/* WM_MOUSEHOOK_SCROLL_MODE */
LRESULT scroll_mode(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct mode_conf *data;

    ctx.scroll_data.start_pt.x = LOWORD(wparam);
    ctx.scroll_data.start_pt.y = HIWORD(wparam);

    data = (struct mode_conf *)lparam;

    if(data->mode != 1) {
        return 0;
    }

    ctx.scroll_data.x_ratio = data->scroll.x_ratio;
    ctx.scroll_data.y_ratio = data->scroll.y_ratio;

    ctx.scroll_data.target = WindowFromPoint(ctx.scroll_data.start_pt);

    {
        TCHAR class[256];

        memset(class, 0, sizeof(class));
        GetClassName(ctx.scroll_data.target, class, 256);

        if(_tcscmp(class, _T("SysListView32")) == 0) {
            ctx.scroll_data.mode = SCROLL_MODE_LINESCRL;
        } else if(_tcscmp(class, _T("Internet Explorer_Server")) == 0) {
            HRESULT hres;

            ctx.scroll_data.mode = SCROLL_MODE_IE;

            hres = get_ie_target(ctx.scroll_data.target,
                                 ctx.scroll_data.start_pt.x,
                                 ctx.scroll_data.start_pt.y,
                                 &ctx.scroll_data.ie_target);
            if(FAILED(hres)) {
                ctx.scroll_data.mode = SCROLL_MODE_LINESCRL;
            }
        } else {
            LONG_PTR style;

            style = GetWindowLongPtr(ctx.scroll_data.target, GWL_STYLE);

            if((style & WS_HSCROLL) && (style & WS_VSCROLL)) {
                ctx.scroll_data.mode = SCROLL_MODE_NATIVE_HV;
            } else if(style & WS_HSCROLL) {
                ctx.scroll_data.mode = SCROLL_MODE_NATIVE_H;
            } else if(style & WS_VSCROLL) {
                ctx.scroll_data.mode = SCROLL_MODE_NATIVE_V;
            } else {
                ctx.scroll_data.mode = SCROLL_MODE_WHEEL;
            }
        }
    }

    if(ctx.scroll_data.mode != SCROLL_MODE_IE ||
       FAILED(get_ie_elem_size(ctx.scroll_data.ie_target,
                               &ctx.scroll_data.target_size)) ||
       FAILED(init_ie_dpids(ctx.scroll_data.ie_target))) {
        RECT rt;

        GetClientRect(ctx.scroll_data.target, &rt);

        ctx.scroll_data.target_size.cx = rt.right - rt.left + 1;
        ctx.scroll_data.target_size.cy = rt.bottom - rt.top + 1;
    }

    ctx.scroll_data.dx = 0;
    ctx.scroll_data.dy = 0;

    ctx.scroll_data.dw = 0;

    return 0;
}

/* WM_MOUSEHOOK_SCROLLING */
LRESULT scrolling(HWND hwnd, UINT msgid, WPARAM wparam, LPARAM lparam)
{
    int x, y;
    MSG msg;

    typedef void (*mode_proc_t)(int, int);
    mode_proc_t proc;

    static struct uint_ptr_pair mode_map[] = {
        {SCROLL_MODE_NATIVE_HV, scrolling_native_hv},
        {SCROLL_MODE_NATIVE_H, scrolling_native_h},
        {SCROLL_MODE_NATIVE_V, scrolling_native_v},
        {SCROLL_MODE_LINESCRL, scrolling_line},
        {SCROLL_MODE_IE, scrolling_ie},
        {SCROLL_MODE_WHEEL, scrolling_wheel},

        {0, NULL}
    };

    x = LOWORD(wparam);
    y = HIWORD(wparam);

    ctx.scroll_data.dx += (x - ctx.scroll_data.start_pt.x) *
                          ctx.scroll_data.x_ratio;
    ctx.scroll_data.dy += (y - ctx.scroll_data.start_pt.y) *
                          ctx.scroll_data.y_ratio;

    while(PeekMessage(&msg, hwnd,
                      WM_MOUSEHOOK_SCROLL_MODE, WM_MOUSEHOOK_SCROLLING,
                      PM_NOREMOVE)) {
        if(msg.message != WM_MOUSEHOOK_SCROLLING) {
            break;
        }

        x = LOWORD(msg.wParam);
        y = HIWORD(msg.wParam);

        ctx.scroll_data.dx += (x - ctx.scroll_data.start_pt.x) *
                              ctx.scroll_data.x_ratio;
        ctx.scroll_data.dy += (y - ctx.scroll_data.start_pt.y) *
                              ctx.scroll_data.y_ratio;

        /* discard */
        PeekMessage(&msg, hwnd,
                    WM_MOUSEHOOK_SCROLL_MODE, WM_MOUSEHOOK_SCROLLING,
                    PM_REMOVE);
    }

    proc = assq_pair(mode_map, ctx.scroll_data.mode, NULL);
    if(proc) {
        proc(x, y);
    }

    return 0;
}