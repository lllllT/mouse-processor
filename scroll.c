/*
 * scroll.c  -- scroll window
 *
 * $Id: scroll.c,v 1.1 2004/12/31 18:55:51 hos Exp $
 *
 */

#include <windows.h>
#include <tchar.h>

#include "main.h"


static
int scroll_window(int bar, int delta, int length)
{
    SCROLLINFO si;
    int min, max, pos;
    double spd;

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    if(GetScrollInfo(ctx.scroll_data.target, bar, &si) == 0) {
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
        return 0;
    }

    pos += delta;
    if(pos < min) {
        pos = min;
    } else if(pos > max) {
        pos = max;
    }

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = pos;
    SetScrollInfo(ctx.scroll_data.target, bar, &si, FALSE);
    SendMessageTimeout(ctx.scroll_data.target,
                       (bar == SB_HORZ ? WM_HSCROLL : WM_VSCROLL),
                       MAKEWPARAM(SB_THUMBTRACK, pos), 0,
                       SMTO_ABORTIFHUNG, 500, NULL);

    return 1;
}


LRESULT scroll_begin(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ctx.scroll_data.start_pt.x = wparam;
    ctx.scroll_data.start_pt.y = lparam;

    ctx.scroll_data.target = WindowFromPoint(ctx.scroll_data.start_pt);

    {
        TCHAR class[256];

        memset(class, 0, sizeof(class));
        GetClassName(ctx.scroll_data.target, class, 256);

        if(_tcscmp(class, _T("SysListView32")) == 0) {
            ctx.scroll_data.mode = SCROLL_MODE_LISTVIEW;
        } else if(_tcscmp(class, _T("Internet Explorer_Server")) == 0) {
            ctx.scroll_data.mode = SCROLL_MODE_IE;
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

    {
        RECT rt;

        GetClientRect(ctx.scroll_data.target, &rt);

        ctx.scroll_data.target_size.cx = rt.right - rt.left + 1;
        ctx.scroll_data.target_size.cy = rt.bottom - rt.top + 1;
    }

    ctx.scroll_data.dx = 0;
    ctx.scroll_data.dy = 0;

    return 0;
}

LRESULT scrolling(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int x, y;

    x = wparam;
    y = lparam;

    ctx.scroll_data.dx += x - ctx.scroll_data.start_pt.x;
    ctx.scroll_data.dy += y - ctx.scroll_data.start_pt.y;

    if(ctx.scroll_data.mode == SCROLL_MODE_NATIVE_V ||
       ctx.scroll_data.mode == SCROLL_MODE_NATIVE_HV) {
        if(scroll_window(SB_VERT,
                         ctx.scroll_data.dy, ctx.scroll_data.target_size.cy)) {
            ctx.scroll_data.dy = 0;
        }
    }

    if(ctx.scroll_data.mode == SCROLL_MODE_NATIVE_H ||
       ctx.scroll_data.mode == SCROLL_MODE_NATIVE_HV) {
        if(scroll_window(SB_HORZ,
                         ctx.scroll_data.dx, ctx.scroll_data.target_size.cx)) {
            ctx.scroll_data.dx = 0;
        }
    }

    return 0;
}

LRESULT scroll_end(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return 0;
}
