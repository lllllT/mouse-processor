/*
 * hook.c  -- hook funcs
 *
 * $Id: hook.c,v 1.1 2004/12/27 05:40:14 hos Exp $
 *
 */

#include <windows.h>

#include "main.h"

#define LLMHF_INJECTED 0x00000001


static HHOOK mouse_ll_hook = NULL;


static
LRESULT CALLBACK mouse_ll_proc(int code, WPARAM wparam, LPARAM lparam)
{
    MSLLHOOKSTRUCT *msll;

    if(code < 0) {
        goto norm_end;
    }

    msll = (MSLLHOOKSTRUCT *)lparam;

    if(msll->flags & LLMHF_INJECTED) {
        goto norm_end;
    }

    {
        static int scroll_mode = 0, btn_stat = 0;
        static POINT start_pt;
        static HWND target;
        static double h_spd, v_spd; /* scroll-unit per dot */
        static int hmin, hmax, hpos;
        static int vmin, vmax, vpos;
        static int dx, dy;

        switch(wparam) {
          case WM_RBUTTONDOWN:
              if(scroll_mode) {
                  btn_stat |= 0x01;
                  return 1;
              }

              if(GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                  RECT rt;

                  start_pt = msll->pt;
                  target = WindowFromPoint(start_pt);

                  if(GetClientRect(target, &rt) != 0) {
                      SCROLLINFO hsi, vsi;

                      memset(&hsi, 0, sizeof(hsi));
                      hsi.cbSize = sizeof(hsi);
                      hsi.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
                      if(GetScrollInfo(target, SB_HORZ, &hsi) != 0) {
                          int range;
                          double page_ratio;

                          hmin = hsi.nMin;
                          hmax = hsi.nMax - (hsi.nPage ? hsi.nPage - 1 : 0);
                          range = hmax - hmin + 1;
                          page_ratio = (double)hsi.nPage /
                                       (hsi.nMax - hsi.nMin + 1);
                          h_spd = (page_ratio == 1 ? 0 :
                                   (range / ((rt.right - rt.left + 1) *
                                              (1 - page_ratio))));
                          hpos = hsi.nPos;
                      } else {
                          h_spd = 0;
                      }

                      memset(&vsi, 0, sizeof(vsi));
                      vsi.cbSize = sizeof(vsi);
                      vsi.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
                      if(GetScrollInfo(target, SB_VERT, &vsi) != 0) {
                          int range;
                          double page_ratio;

                          vmin = vsi.nMin;
                          vmax = vsi.nMax - (vsi.nPage ? vsi.nPage - 1 : 0);
                          range = vmax - vmin + 1;
                          page_ratio = (double)vsi.nPage /
                                       (vsi.nMax - vsi.nMin + 1);
                          v_spd = (page_ratio == 1 ? 0 :
                                   (range / ((rt.bottom - rt.top + 1) *
                                              (1 - page_ratio))));
                          vpos = vsi.nPos;
                      } else {
                          v_spd = 0;
                      }

                      btn_stat = 0;
                      scroll_mode = 1;
                      dx = dy = 0;
                      return 1;
                  }
              }
              break;

          case WM_RBUTTONUP:
              if(scroll_mode) {
                  if(btn_stat & 0x01) {
                      btn_stat &= ~0x01;
                      scroll_mode = 0;
                  } else {
                      INPUT in;

                      memset(&in, 0, sizeof(in));
                      in.type = INPUT_MOUSE;
                      in.mi.dx = msll->pt.x;
                      in.mi.dy = msll->pt.y;
                      in.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                      in.mi.time = msll->time;
                      in.mi.dwExtraInfo = msll->dwExtraInfo;
                      SendInput(1, &in, sizeof(INPUT));
                  }
                  return 1;
              }
              break;

          case WM_LBUTTONDOWN:
              if(scroll_mode) {
                  btn_stat |= 0x02;
                  return 1;
              }
              break;

          case WM_LBUTTONUP:
              if(scroll_mode) {
                  if(btn_stat & 0x02) {
                      btn_stat &= ~0x02;
                      scroll_mode = 0;
                  }
                  return 1;
              }
              if(scroll_mode & (0x01 | 0x03)) {
                  scroll_mode &= ~0x03;
                  return 1;
              }
              break;

          case WM_MOUSEMOVE:
              if(scroll_mode) {
                  int dh, dv;

                  dx += msll->pt.x - start_pt.x;
                  dy += msll->pt.y - start_pt.y;

                  dh = dx * h_spd;
                  dv = dy * v_spd;

                  if(dh) {
                      hpos += dh;
                      if(hpos < hmin) {
                          hpos = hmin;
                      } else if(hpos > hmax) {
                          hpos = hmax;
                      }

                      dx = 0;

                      SetScrollPos(target, SB_HORZ, hpos, FALSE);
                      PostMessage(target, WM_HSCROLL,
                                  MAKEWPARAM(SB_THUMBTRACK, hpos), 0);
                  }
                  if(dv) {
                      vpos += dv;
                      if(vpos < vmin) {
                          vpos = vmin;
                      } else if(vpos > vmax) {
                          vpos = vmax;
                      }

                      dy = 0;

                      SetScrollPos(target, SB_VERT, vpos, FALSE);
                      PostMessage(target, WM_VSCROLL,
                                  MAKEWPARAM(SB_THUMBTRACK, vpos), 0);
                  }

                  return 1;
              }
              break;
        }
    }

  norm_end:
    return CallNextHookEx(mouse_ll_hook, code, wparam, lparam);
}


int set_hook(void)
{
    if(mouse_ll_hook != NULL) {
        return 1;
    }

    mouse_ll_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_ll_proc, instance, 0);
    if(mouse_ll_hook == NULL) {
        return 0;
    }

    return 1;
}

int clear_hook(void)
{
    if(mouse_ll_hook == NULL) {
        return 1;
    }

    {
        int ret;

        ret = UnhookWindowsHookEx(mouse_ll_hook);
        mouse_ll_hook = NULL;

        if(ret == 0) {
            return 0;
        }
    }

    return 1;
}
