/*
 * hook.c  -- hook funcs
 *
 * $Id: hook.c,v 1.7 2004/12/30 15:31:01 hos Exp $
 *
 */

#include <windows.h>

#include "main.h"

#define LLMHF_INJECTED 0x00000001
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#define MOUSEEVENTF_XDOWN 0x0080
#define MOUSEEVENTF_XUP 0x0100


#define COMB_TIMER_ID 1


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


static
void do_action(struct mouse_action *act, MSLLHOOKSTRUCT *msll, int motion)
{
    switch(act->code) {
      case MOUSE_ACT_BUTTON:
      {
          INPUT in;

          memset(&in, 0, sizeof(in));

          in.type = INPUT_MOUSE;
          in.mi.dx = msll->pt.x;
          in.mi.dy = msll->pt.y;
          in.mi.time = msll->time;
          in.mi.dwExtraInfo = msll->dwExtraInfo;
          fill_input(&in, motion, act->data);

          SendInput(1, &in, sizeof(INPUT));

          break;
      }

      case MOUSE_ACT_WHEEL:
      {
          INPUT in;

          memset(&in, 0, sizeof(in));

          in.type = INPUT_MOUSE;
          in.mi.dx = msll->pt.x;
          in.mi.dy = msll->pt.y;
          in.mi.time = msll->time;
          in.mi.dwExtraInfo = msll->dwExtraInfo;
          fill_input(&in, MOTION_WHEEL, HIWORD(msll->mouseData) * act->data);

          SendInput(1, &in, sizeof(INPUT));

          break;
      }

      case MOUSE_ACT_MOVE:
      {
          INPUT in;

          memset(&in, 0, sizeof(in));

          in.type = INPUT_MOUSE;
          in.mi.dx = msll->pt.x;
          in.mi.dy = msll->pt.y;
          in.mi.time = msll->time;
          in.mi.dwExtraInfo = msll->dwExtraInfo;
          fill_input(&in, MOTION_MOVE, 0);

          SendInput(1, &in, sizeof(INPUT));

          break;
      }

      case MOUSE_ACT_MODECH:
          ctx.conf = (struct mouse_conf *)act->data;
          break;

      case MOUSE_ACT_SCROLL:
          break;
    }
}


static
void CALLBACK comb_timer(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
    KillTimer(hwnd, id);

    if(! ctx.pressed) {
        return;
    }
    ctx.pressed = 0;

    do_action(&ctx.conf->button[ctx.pressed_btn].act,
              &ctx.pressed_btn_data, MOTION_DOWN);
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
            /* check ignore mask */
            if(ctx.ignore_btn_mask & MOUSE_BTN_BIT(btn)) {
                ctx.ignore_btn_mask &= ~MOUSE_BTN_BIT(btn);
                return 1;
            }

            if(ctx.pressed) {
                ctx.pressed = 0;
                KillTimer(ctx.main_window, COMB_TIMER_ID);

                if(motion == MOTION_DOWN) {
                    /* combination press */
                    if(ctx.conf->button[ctx.pressed_btn].comb_act[btn].code !=
                       MOUSE_ACT_NONE) {
                        do_action(
                            &ctx.conf->button[ctx.pressed_btn].comb_act[btn],
                            msll, MOTION_DOWN);

                        if(ctx.conf->button[ctx.pressed_btn].comb_act[btn].code
                           == MOUSE_ACT_BUTTON) {
                            ctx.combination[ctx.combinated * 2] =
                                ctx.pressed_btn;
                            ctx.combination[ctx.combinated * 2 + 1] = btn;
                            ctx.combinated += 1;
                        } else {
                            ctx.ignore_btn_mask |=
                                MOUSE_BTN_BIT(ctx.pressed_btn) |
                                MOUSE_BTN_BIT(btn);
                        }

                        return 1;
                    }
                }

                do_action(&ctx.conf->button[ctx.pressed_btn].act,
                          &ctx.pressed_btn_data, MOTION_DOWN);
            }

            if(motion == MOTION_DOWN) {
                /* try combination */
                if(ctx.conf->button[btn].flags &
                   MOUSE_BTN_CONF_ENABLE_COMB) {
                    ctx.pressed = 1;
                    ctx.pressed_btn = btn;
                    memcpy(&ctx.pressed_btn_data, msll,
                           sizeof(MSLLHOOKSTRUCT));
                    SetTimer(ctx.main_window, COMB_TIMER_ID,
                             ctx.conf->comb_time, comb_timer);
                    return 1;
                }
            }

            if(motion == MOTION_UP) {
                if(ctx.combinated) {
                    int i;

                    for(i = 0; i < ctx.combinated; i++) {
                        /* combination release */
                        if(ctx.combination[i * 2] == btn ||
                           ctx.combination[i * 2 + 1] == btn) {
                            do_action(
                                &ctx.conf->button[ctx.combination[i * 2]].
                                comb_act[ctx.combination[i * 2 + 1]],
                                msll, MOTION_UP);

                            ctx.ignore_btn_mask |= 
                                MOUSE_BTN_BIT(btn == ctx.combination[i * 3] ?
                                              ctx.combination[i * 3 + 1] :
                                              ctx.combination[i * 3]);

                            memmove(&ctx.combination[i * 2],
                                    &ctx.combination[(i + 1) * 2],
                                    sizeof(ctx.combination[0]) *
                                    (ctx.combinated - i - 1));
                            ctx.combinated -= 1;

                            return 1;
                        }
                    }
                }
            }

            /* generic action */
            do_action(&ctx.conf->button[btn].act,
                      msll, motion);

            return 1;
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
