/*
 * hook.c  -- hook funcs
 *
 * $Id: hook.c,v 1.2 2004/12/27 09:22:13 hos Exp $
 *
 */

#include <windows.h>

#include "main.h"

#define LLMHF_INJECTED 0x00000001


static HHOOK mouse_ll_hook = NULL;


static
int msg_to_motion(WPARAM msg)
{
    switch(msg) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_XBUTTONDOWN:
          return MOTION_DOWN;

      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_MBUTTONUP:
      case WM_XBUTTONUP:
          return MOTION_UP;

      case WM_MOUSEMOVE:
          return MOTION_MOVE;

      case WM_MOUSEWHEEL:
          return MOTION_WHEEL;
    }

    return -1;
}

static
int msg_to_button(WPARAM msg, DWORD mouse_data)
{
    switch(msg) {
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
          return 0;

      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
          return 1;

      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
          return 2;

      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP:
          switch(HIWORD(mouse_data)) {
            case XBUTTON1:
                return 3;

            case XBUTTON2:
                return 4;
          }
    }

    return -1;
}

static
void fill_input(INPUT *in, int motion, int data)
{
    switch(motion) {
      case MOTION_DOWN:
          switch(data) {
            case 0:
                in->mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                break;

            case 1:
                in->mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                break;

            case 2:
                in->mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                break;

            case 3:
                in->mi.dwFlags = MOUSEEVENTF_XDOWN;
                in->mi.mouseData = XBUTTON1;
                break;

            case 4:
                in->mi.dwFlags = MOUSEEVENTF_XDOWN;
                in->mi.mouseData = XBUTTON2;
                break;
          }
          break;

      case MOTION_UP:
          switch(data) {
            case 0:
                in->mi.dwFlags = MOUSEEVENTF_LEFTUP;
                break;

            case 1:
                in->mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                break;

            case 2:
                in->mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
                break;

            case 3:
                in->mi.dwFlags = MOUSEEVENTF_XUP;
                in->mi.mouseData = XBUTTON1;
                break;

            case 4:
                in->mi.dwFlags = MOUSEEVENTF_XUP;
                in->mi.mouseData = XBUTTON2;
                break;
          }
          break;

      case MOTION_MOVE:
          in->mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
          break;

      case MOTION_WHEEL:
          in->mi.dwFlags = MOUSEEVENTF_WHEEL;
          in->mi.mouseData = data;
          break;
    }
}


static inline
int post_main(int motion, int data)
{
    PostMessage(ctx.main_window, WM_MOUSEHOOK_MOTION, motion, data);
}


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
        int motion, btn;

        motion = msg_to_motion(wparam);
        btn = msg_to_button(wparam, msll->mouseData);

        if(btn >= 0) {
            if(! ctx.pressed) {
                if(ctx.conf.button[btn].flags & MOUSE_BTN_CONF_ENABLE_COMB) {
                    if(motion == MOTION_DOWN) {
                        ctx.pressed = 1;
                        memcpy(&ctx.pressed_btn, msll, sizeof(MSLLHOOKSTRUCT));
                        post_to_main(motion, btn);
                        return 1;
                    } else {
                        INPUT in[2];
                        int send_btn;

                        send_btn = (ctx.conf.button[btn].flags &
                                    MOUSE_BTN_CONF_REPLACE ?
                                    ctx.conf.button[btn].replace : btn);

                        memset(&in, 0, sizeof(in));

                        in[0].type = INPUT_MOUSE;
                        in[0].mi.dx = ctx.pressed_btn.pt.x;
                        in[0].mi.dy = ctx.pressed_btn.pt.y;
                        in[0].mi.time = ctx.pressed_btn.time;
                        in[0].mi.dwExtraInfo = ctx.pressed_btn.dwExtraInfo;
                        fill_input(&in[0], MOTION_DOWN, send_btn);

                        in[1].type = INPUT_MOUSE;
                        in[1].mi.dx = msll->pt.x;
                        in[1].mi.dy = msll->pt.y;
                        in[1].mi.time = msll->time;
                        in[1].mi.dwExtraInfo = msll->dwExtraInfo;
                        fill_input(&in[1], MOTION_UP, send_btn);

                        SendInput(2, &in, sizeof(INPUT));
                        ctx.pressed = 0;
                        return 1;
                    }
                }
                if(ctx.conf.button[btn].flags & MOUSE_BTN_CONF_REPLACE) {
                    INPUT in;

                    memset(&in, 0, sizeof(in));

                    in.type = INPUT_MOUSE;
                    in.mi.dx = msll->pt.x;
                    in.mi.dy = msll->pt.y;
                    in.mi.time = msll->time;
                    in.mi.dwExtraInfo = msll->dwExtraInfo;
                    fill_input(&in, motion, ctx.conf.button[btn].replace);

                    SendInput(1, &in, sizeof(INPUT));
                    ctx.pressed = 0;
                    return 1;
                }

                goto norm_end;
            }
        }

        if(btn < 0) {
            goto norm_end;
        }
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

    mouse_ll_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_ll_proc,
                                     ctx.instance, 0);
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
