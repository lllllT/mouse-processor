/*
 * hook.c  -- hook funcs
 *
 * $Id: hook.c,v 1.13 2005/01/07 05:50:00 hos Exp $
 *
 */

#include "main.h"
#include <process.h>

#define LLMHF_INJECTED 0x00000001
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#define MOUSEEVENTF_XDOWN 0x0080
#define MOUSEEVENTF_XUP 0x0100
#define MOUSEEVENTF_VIRTUALDESK 0x4000


#define COMB_TIMER_ID 1

#define HOOK_THREAD_END (WM_APP + 100)


static HHOOK mouse_ll_hook = NULL;
static DWORD hook_thread_id = 0;
static HANDLE hook_thread_end_evt = NULL;


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
          fill_input(&in, motion, act->conf.button);

          SendInput(1, &in, sizeof(INPUT));

          break;
      }

      case MOUSE_ACT_WHEEL:
      {
          INPUT in;
          int data, wn, i;

          act->data.wheel +=
              (short)HIWORD(msll->mouseData) * act->conf.wheel.ratio;
          wn = act->data.wheel / act->conf.wheel.tick;
          if(wn == 0) {
              break;
          }

          data = act->conf.wheel.tick;
          if(wn < 0) {
              wn = -wn;
              data = -data;
          }

          memset(&in, 0, sizeof(in));

          in.type = INPUT_MOUSE;
          in.mi.time = msll->time;
          in.mi.dwExtraInfo = msll->dwExtraInfo;
          fill_input(&in, MOTION_WHEEL, data);

          for(i = 0; i < wn; i++) {
              SendInput(1, &in, sizeof(INPUT));
          }

          act->data.wheel -= data * wn;

          break;
      }

      case MOUSE_ACT_WHEELPOST:
      {
          int wn, data, i;
          HWND target;

          act->data.wheel +=
              (short)HIWORD(msll->mouseData) * act->conf.wheel.ratio;

          target = WindowFromPoint(msll->pt);
          if(target == NULL) {
              break;
          }

          wn = act->data.wheel / act->conf.wheel.tick;
          if(wn == 0) {
              break;
          }

          data = act->conf.wheel.tick;
          if(wn < 0) {
              wn = -wn;
              data = -data;
          }

          for(i = 0; i < wn; i++) {

#define KEY_STATE(key) (GetAsyncKeyState(VK_ ## key) & 0x8000 ? MK_ ## key : 0)

              PostMessage(
                  target, WM_MOUSEWHEEL,
                  MAKEWPARAM(KEY_STATE(CONTROL) |
                             KEY_STATE(LBUTTON) |
                             KEY_STATE(MBUTTON) |
                             KEY_STATE(RBUTTON) |
                             KEY_STATE(SHIFT) |
                             KEY_STATE(XBUTTON1) |
                             KEY_STATE(XBUTTON2),
                             data),
                  MAKELPARAM(msll->pt.x, msll->pt.y));

#undef KEY_STATE

          act->data.wheel -= data * wn;

          }

          break;
      }

      case MOUSE_ACT_MOVE:
          break;

      case MOUSE_ACT_MODECH:
          ctx.conf = act->conf.mode.mode;
          PostMessage(ctx.main_window, WM_MOUSEHOOK_SCROLL_MODE,
                      MAKEWPARAM(msll->pt.x, msll->pt.y),
                      (LPARAM)&act->conf.mode.data);
          break;

      case MOUSE_ACT_SCROLL:
          PostMessage(ctx.main_window, WM_MOUSEHOOK_SCROLLING,
                      MAKEWPARAM(msll->pt.x, msll->pt.y), 0);
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
                             ctx.comb_time, comb_timer);
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


struct hook_data {
    int ret;
    HANDLE start_evt;
};

static
void __cdecl hook_thread(void *arg)
{
    struct hook_data *data = (struct hook_data *)arg;
    MSG msg;

    /* create thread message queue */
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    hook_thread_id = GetCurrentThreadId();

    mouse_ll_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_ll_proc,
                                     ctx.instance, 0);
    if(mouse_ll_hook == NULL) {
        hook_thread_id = 0;
        data->ret = 0;
    } else {
        data->ret = 1;
    }
    SetEvent(data->start_evt);

    while(hook_thread_id > 0) {
        if(GetMessage(&msg, NULL, 0, 0) < 0) {
            break;
        }

        if(msg.message == HOOK_THREAD_END) {
            UnhookWindowsHookEx(mouse_ll_hook);
            mouse_ll_hook = NULL;

            SetEvent(hook_thread_end_evt);
            break;
        }
    }

    hook_thread_id = 0;
    _endthread();
}

int set_hook(void)
{
    if(mouse_ll_hook != NULL) {
        return 1;
    }

    {
        struct hook_data data;
        long ret;

        memset(&data, 0, sizeof(data));

        data.start_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
        hook_thread_end_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(data.start_evt == NULL || hook_thread_end_evt == NULL) {
            return 0;
        }

        ret = (long)_beginthread(hook_thread, 0, &data);
        if(ret == -1) {
            CloseHandle(data.start_evt);
            return 0;
        }

        ret = WaitForSingleObject(data.start_evt, INFINITE);
        CloseHandle(data.start_evt);
        if(ret != WAIT_OBJECT_0) {
            return 0;
        }

        return data.ret;
    }
}

int clear_hook(void)
{
    if(mouse_ll_hook == NULL) {
        return 1;
    }

    PostThreadMessage(hook_thread_id, HOOK_THREAD_END, 0, 0);
    WaitForSingleObject(hook_thread_end_evt, INFINITE);
    CloseHandle(hook_thread_end_evt);

    return 1;
}
