/*
 * hook.c  -- hook funcs
 *
 * $Id: hook.c,v 1.20 2005/01/17 06:14:29 hos Exp $
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


#define HOOK_THREAD_END (WM_APP + 100)


static HHOOK mouse_ll_hook = NULL;
static DWORD hook_thread_id = 0;
static HANDLE hook_thread_end_evt = NULL;

static UINT_PTR comb_timer_id = 0;


inline static
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

inline static
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

inline static
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

inline static
void do_action(struct mouse_action *act, MSLLHOOKSTRUCT *msll, int motion)
{
    switch(act->code) {
      case MOUSE_ACT_BUTTON:
          if(motion == MOTION_DOWN || motion == MOTION_UP) {
              INPUT in;

              memset(&in, 0, sizeof(in));

              in.type = INPUT_MOUSE;
              in.mi.time = msll->time;
              in.mi.dwExtraInfo = msll->dwExtraInfo;
              fill_input(&in, motion, act->conf.button);

              SendInput(1, &in, sizeof(INPUT));
          }
          break;

      case MOUSE_ACT_WHEEL:
          break;

      case MOUSE_ACT_WHEELPOST:
          if(motion == MOTION_WHEEL) {
              HWND target;

              target = WindowFromPoint(msll->pt);
              if(target == NULL) {
                  break;
              }

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
                             (short)HIWORD(msll->mouseData)),
                  MAKELPARAM(msll->pt.x, msll->pt.y));

#undef KEY_STATE
          }
          break;

      case MOUSE_ACT_MOVE:
          break;

      case MOUSE_ACT_MODECH:
          if(motion == MOTION_UP) {
              ctx.mode_data.cur_conf = act->conf.mode_change.mode;
              PostMessage(ctx.main_window, WM_MOUSEHOOK_MODECH,
                          MAKEWPARAM(msll->pt.x, msll->pt.y),
                          (LPARAM)&act->conf.mode_change.data);
          }
          break;

      case MOUSE_ACT_MODEMSG:
          if(motion == MOTION_UP || motion == MOTION_MOVE) {
              PostMessage(ctx.main_window, WM_MOUSEHOOK_MODEMSG,
                          MAKEWPARAM(msll->pt.x, msll->pt.y),
                          (LPARAM)&act->conf.mode_msg.data);
          }
          break;

      case MOUSE_ACT_NONE:
          /* do nothing */
          break;
    }
}


static
void CALLBACK comb_timer(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
    KillTimer(hwnd, id);

    if(! ctx.hook_data.pressed) {
        return;
    }
    ctx.hook_data.pressed = 0;

    do_action(&ctx.mode_data.cur_conf->button[ctx.hook_data.pressed_btn].act,
              &ctx.hook_data.pressed_btn_data, MOTION_DOWN);
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

    if(ctx.mode_data.cur_conf == NULL) {
        goto norm_end;
    }

    {
        int motion, btn;

        motion = msg_to_motion(wparam);
        btn = msg_to_button(wparam, msll->mouseData);

        if(ctx.hook_data.pressed) {
            ctx.hook_data.pressed = 0;
            KillTimer(NULL, comb_timer_id);

            if(motion == MOTION_DOWN) {
                /* combination press */
                if(ctx.mode_data.cur_conf->
                   button[ctx.hook_data.pressed_btn].comb_act[btn].code !=
                   MOUSE_ACT_NONE) {
                    do_action(
                        &ctx.mode_data.cur_conf->
                        button[ctx.hook_data.pressed_btn].comb_act[btn],
                        msll, MOTION_DOWN);

                    ctx.hook_data.
                        combination[ctx.hook_data.combinated * 2] =
                        ctx.hook_data.pressed_btn;
                    ctx.hook_data.
                        combination[ctx.hook_data.combinated * 2 + 1] = btn;
                    ctx.hook_data.combinated += 1;

                    return 1;
                }
            }

            do_action(&ctx.mode_data.cur_conf->
                      button[ctx.hook_data.pressed_btn].act,
                      &ctx.hook_data.pressed_btn_data, MOTION_DOWN);
        }

        if(btn >= 0) {
            /* check ignore mask */
            if(ctx.hook_data.ignore_btn_mask & MOUSE_BTN_BIT(btn)) {
                ctx.hook_data.ignore_btn_mask &= ~MOUSE_BTN_BIT(btn);
                return 1;
            }

            if(motion == MOTION_DOWN) {
                /* try combination */
                if(ctx.mode_data.cur_conf->button[btn].flags &
                   MOUSE_BTN_CONF_ENABLE_COMB) {
                    ctx.hook_data.pressed = 1;
                    ctx.hook_data.pressed_btn = btn;
                    memcpy(&ctx.hook_data.pressed_btn_data, msll,
                           sizeof(MSLLHOOKSTRUCT));
                    comb_timer_id = SetTimer(NULL, 0,
                                             ctx.app_conf.comb_time,
                                             comb_timer);
                    return 1;
                }
            }

            if(motion == MOTION_UP) {
                if(ctx.hook_data.combinated) {
                    int i;

                    for(i = 0; i < ctx.hook_data.combinated; i++) {
                        /* combination release */
                        if(ctx.hook_data.combination[i * 2] == btn ||
                           ctx.hook_data.combination[i * 2 + 1] == btn) {
                            do_action(
                                &ctx.mode_data.cur_conf->
                                button[ctx.hook_data.combination[i * 2]].
                                comb_act[ctx.hook_data.combination[i * 2 + 1]],
                                msll, MOTION_UP);

                            ctx.hook_data.ignore_btn_mask |= 
                                MOUSE_BTN_BIT(
                                    btn == ctx.hook_data.combination[i * 3] ?
                                    ctx.hook_data.combination[i * 3 + 1] :
                                    ctx.hook_data.combination[i * 3]);

                            memmove(&ctx.hook_data.combination[i * 2],
                                    &ctx.hook_data.combination[(i + 1) * 2],
                                    sizeof(ctx.hook_data.combination[0]) *
                                    (ctx.hook_data.combinated - i - 1));
                            ctx.hook_data.combinated -= 1;

                            return 1;
                        }
                    }
                }
            }

            /* generic action */
            do_action(&ctx.mode_data.cur_conf->button[btn].act,
                      msll, motion);

            return 1;
        }

        if(motion == MOTION_WHEEL) {
            if(ctx.mode_data.cur_conf->wheel_act.code == MOUSE_ACT_WHEEL) {
                goto norm_end;
            }

            do_action(&ctx.mode_data.cur_conf->wheel_act, msll, MOTION_WHEEL);

            return 1;
        }

        if(motion == MOTION_MOVE) {
            if(ctx.mode_data.cur_conf->move_act.code == MOUSE_ACT_MOVE) {
                goto norm_end;
            }

            do_action(&ctx.mode_data.cur_conf->move_act, msll, MOTION_MOVE);

            return 1;
        }
    }

  norm_end:
    return CallNextHookEx(mouse_ll_hook, code, wparam, lparam);
}


struct hook_data {
    int ret;
    DWORD last_error;
    HANDLE start_evt;
};

static
void __cdecl hook_thread(void *arg)
{
    struct hook_data *data = (struct hook_data *)arg;
    MSG msg;
    int ret;

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
    data->last_error = GetLastError();
    SetEvent(data->start_evt);

    while(hook_thread_id > 0) {
        ret = GetMessage(&msg, NULL, 0, 0);
        if(ret < 0) {
            break;
        }

        if(ret == 0 ||
           msg.message == HOOK_THREAD_END) {
            UnhookWindowsHookEx(mouse_ll_hook);
            mouse_ll_hook = NULL;

            SetEvent(hook_thread_end_evt);
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
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

        SetLastError(data.last_error);
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
