/*
 * hook.c  -- hook funcs
 *
 * $Id: hook.c,v 1.9 2004/12/31 18:57:12 hos Exp $
 *
 */

#include <windows.h>
#include <stdio.h>

#include "main.h"

#define LLMHF_INJECTED 0x00000001
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#define MOUSEEVENTF_XDOWN 0x0080
#define MOUSEEVENTF_XUP 0x0100
#define MOUSEEVENTF_VIRTUALDESK 0x4000


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
          in.mi.time = msll->time;
          in.mi.dwExtraInfo = msll->dwExtraInfo;
          fill_input(&in, MOTION_WHEEL,
                     (short)HIWORD(msll->mouseData) * act->data);

          SendInput(1, &in, sizeof(INPUT));

          break;
      }

      case MOUSE_ACT_MOVE:
          break;

      case MOUSE_ACT_MODECH:
          if(ctx.conf == &ctx.norm_conf) {
              ctx.conf = &ctx.scroll_conf;
              PostMessage(ctx.main_window, WM_MOUSEHOOK_SCROLL_BEGIN,
                          msll->pt.x, msll->pt.y);
          } else {
              ctx.conf = &ctx.norm_conf;
              PostMessage(ctx.main_window, WM_MOUSEHOOK_SCROLL_END,
                          msll->pt.x, msll->pt.y);
          }
          break;

      case MOUSE_ACT_SCROLL:
          PostMessage(ctx.main_window, WM_MOUSEHOOK_SCROLLING,
                      msll->pt.x, msll->pt.y);
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

        if(btn >= 0) {
            /* check ignore mask */
            if(ctx.ignore_btn_mask & MOUSE_BTN_BIT(btn)) {
                ctx.ignore_btn_mask &= ~MOUSE_BTN_BIT(btn);
                return 1;
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

        if(motion == MOTION_WHEEL) {
            do_action(&ctx.conf->wheel_act, msll, MOTION_WHEEL);

            return 1;
        }

        if(motion == MOTION_MOVE) {
            if(ctx.conf->move_act.code != MOUSE_ACT_MOVE) {
                do_action(&ctx.conf->move_act, msll, MOTION_MOVE);

                return 1;
            }
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
