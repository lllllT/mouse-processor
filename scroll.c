/*
 * scroll.c  -- scroll window
 *
 * $Id: scroll.c,v 1.20 2005/01/18 09:36:41 hos Exp $
 *
 */

#include "main.h"
#include "util.h"
#include <commctrl.h>
#include <psapi.h>


static
void get_hierarchial_window_class_title(HWND hwnd,
                                        LPWSTR *class, LPWSTR *title)
{
    HWND w;
    WCHAR buf[256];
    WCHAR *cp = NULL, *tp = NULL, *p, *t;
    int clen = 0, tlen = 0;

    w = hwnd;
    while(w) {
        /* class */
        buf[0] = 0;
        GetClassNameW(w, buf, 256);

        clen += wcslen(buf) + 2;

        p = (WCHAR *)malloc(clen * sizeof(WCHAR));
        if(p == NULL) {
            free(cp);
            free(tp);
            return;
        }
        wcscpy(p, L":");
        wcscat(p, buf);
        if(cp != NULL) {
            wcscat(p, cp);
            free(cp);
        }
        cp = p;

        /* title */
        buf[0] = 0;
        GetWindowTextW(w, buf, 256);
        t = wcschr(buf, L'\n');
        if(t != NULL) t[0] = 0;

        tlen += wcslen(buf) + 2;

        p = (WCHAR *)malloc(tlen * sizeof(WCHAR));
        if(p == NULL) {
            free(cp);
            free(tp);
            return;
        }
        wcscpy(p, L":");
        wcscat(p, buf);
        if(tp != NULL) {
            wcscat(p, tp);
            free(tp);
        }
        tp = p;

        /* next window */
        w = GetParent(w);
    }

    /* window module filename */
    memset(buf, 0, sizeof(buf));
    {
        HINSTANCE inst;
        DWORD pid = 0;
        HANDLE ph;

        inst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        GetWindowThreadProcessId(hwnd, &pid);
        ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                         FALSE, pid);
        if(ph != NULL) {
            inst = NULL;
            GetModuleFileNameExW(ph, inst, buf, sizeof(buf));

            CloseHandle(ph);
        }
    }

    /* prepend to class */
    clen += wcslen(buf) + 1;

    p = (WCHAR *)malloc(clen * sizeof(WCHAR));
    if(p == NULL) {
        free(cp);
        free(tp);
        return;
    }
    wcscpy(p, buf);
    if(cp != NULL) {
        wcscat(p, cp);
        free(cp);
    }
    cp = p;

    /* prepend to title */
    tlen += wcslen(buf) + 1;

    p = (WCHAR *)malloc(tlen * sizeof(WCHAR));
    if(p == NULL) {
        free(cp);
        free(tp);
        return;
    }
    wcscpy(p, buf);
    if(tp != NULL) {
        wcscat(p, tp);
        free(tp);
    }
    tp = p;

    /* return value */
    *class = cp;
    *title = tp;
}


struct enum_notr_data {
    POINT spt;
    HWND target;
};

static
BOOL CALLBACK enum_nontr_window(HWND hwnd, LPARAM lparam)
{
    struct enum_notr_data *data;
    LONG_PTR style;
    POINT pt;
    LRESULT ht;

    data = (struct enum_notr_data *)lparam;

    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if((style & WS_VISIBLE) != WS_VISIBLE) {
        return TRUE;
    }

    pt = data->spt;
    ScreenToClient(hwnd, &pt);

    if(RealChildWindowFromPoint(hwnd, pt) == NULL) {
        return TRUE;
    }

    if(SendMessageTimeout(hwnd, WM_NCHITTEST,
                          0, MAKELPARAM(data->spt.x, data->spt.y),
                          SMTO_ABORTIFHUNG, 500, &ht) == 0) {
        return TRUE;
    }

    if(ht == HTTRANSPARENT) {
        return TRUE;
    }

    data->target = hwnd;
    return FALSE;
}

static
HWND get_window_for_mouse_input(POINT spt)
{
    HWND w;
    LRESULT ht;

    w = WindowFromPoint(spt);
    while(1) {
        LONG_PTR style;
        HWND p;

        style = GetWindowLongPtr(w, GWL_STYLE);
        if((style & WS_CHILD) != WS_CHILD) {
            break;
        }

        p = GetParent(w);
        if(p == NULL) {
            break;
        }

        w = p;
    }

    if(SendMessageTimeout(w, WM_NCHITTEST, 0, MAKELPARAM(spt.x, spt.y),
                          SMTO_ABORTIFHUNG, 500, &ht) != 0 &&
       ht == HTTRANSPARENT) {
        DWORD tid;
        struct enum_notr_data data;

        tid = GetWindowThreadProcessId(w, NULL);

        memset(&data, 0, sizeof(data));
        data.spt = spt;
        EnumWindows(enum_nontr_window, (LPARAM)&data);
        if(data.target != NULL) {
            w = data.target;
        }
    }

    while(1) {
        POINT pt;
        HWND c;

        pt = spt;
        ScreenToClient(w, &pt);

        c = RealChildWindowFromPoint(w, pt);
        if(c == w) {
            break;
        }

        if(c == NULL) {
            w = c;
            break;
        }

        w = c;
    }

    return w;
}


static
LRESULT start_scroll_mode(struct mode_conf *data)
{
    int i;
    struct scroll_window_conf *target_win_conf;

    /* x and y ratio */
    ctx.mode_data.scroll.x_ratio = ctx.mode_data.cur_conf->scroll_mode.x_ratio;
    ctx.mode_data.scroll.y_ratio = ctx.mode_data.cur_conf->scroll_mode.y_ratio;

    /* target window */
    ctx.mode_data.scroll.target =
        get_window_for_mouse_input(ctx.mode_data.scroll.start_pt);

    /* hierarchical window class/title */
    ctx.mode_data.scroll.class = NULL;
    ctx.mode_data.scroll.title = NULL;
    get_hierarchial_window_class_title(ctx.mode_data.scroll.target,
                                       &ctx.mode_data.scroll.class,
                                       &ctx.mode_data.scroll.title);

    log_printf(LOG_LEVEL_DEBUG,
               L"\n"
               L"start scroll mode: %ls\n"
               L"scroll ratio: %8.3lf, %8.3lf\n"
               L"window class: %ls\n"
               L"window title: %ls\n",
               ctx.mode_data.cur_conf->mode_name,
               ctx.mode_data.scroll.x_ratio, ctx.mode_data.scroll.y_ratio,
               ctx.mode_data.scroll.class, ctx.mode_data.scroll.title);

    /* search target window configuration */
    {
        BSTR class_re, title_re;
        BSTR class, title;

        class = SysAllocString(ctx.mode_data.scroll.class);
        title = SysAllocString(ctx.mode_data.scroll.title);

        target_win_conf = NULL;
        for(i = 0; i < ctx.app_conf.window_conf_num; i++) {
            class_re =
                SysAllocString(ctx.app_conf.window_conf[i].class_regexp);
            title_re =
                SysAllocString(ctx.app_conf.window_conf[i].title_regexp);

            if(ctx.app_conf.window_conf[i].class_or_title) {
                if((class_re != NULL && is_regexp_match(class_re, class)) ||
                   (title_re != NULL && is_regexp_match(title_re, title))) {
                    target_win_conf = &ctx.app_conf.window_conf[i];

                    SysFreeString(class_re);
                    SysFreeString(title_re);
                    break;
                }
            } else {
                if((class_re != NULL && is_regexp_match(class_re, class)) &&
                   (title_re != NULL && is_regexp_match(title_re, title))) {
                    target_win_conf = &ctx.app_conf.window_conf[i];

                    SysFreeString(class_re);
                    SysFreeString(title_re);
                    break;
                }
            }

            SysFreeString(class_re);
            SysFreeString(title_re);
        }

        SysFreeString(class);
        SysFreeString(title);

        if(target_win_conf == NULL) {
            log_printf(LOG_LEVEL_DEBUG, L"window configuration not match\n");
            return 0;
        }
    }

    log_printf(LOG_LEVEL_DEBUG, L"match: ");
    log_print_s_exp(LOG_LEVEL_DEBUG, target_win_conf->regexp);
    log_printf(LOG_LEVEL_DEBUG, L"\n");

    /* start operator */
    {
        scroll_op_arg_t arg;

        memset(&arg, 0, sizeof(arg));
        arg.conf = target_win_conf->op->conf;
        arg.arg = S_EXP_CDR(target_win_conf->args);
        arg.hwnd = ctx.mode_data.scroll.target;
        arg.pos = ctx.mode_data.scroll.start_pt;

        if(target_win_conf->op->proc.get_context_size != NULL) {
            ctx.mode_data.scroll.op_context_size =
                target_win_conf->op->proc.get_context_size(&arg);
            if(ctx.mode_data.scroll.op_context_size < 0) {
                log_printf(LOG_LEVEL_DEBUG,
                           L"scroll operator: "
                           L"start fail (get_context_size)\n");
                return 0;
            }
        } else {
            ctx.mode_data.scroll.op_context_size = 0;
        }

        ctx.mode_data.scroll.op_context =
            malloc(ctx.mode_data.scroll.op_context_size);
        if(ctx.mode_data.scroll.op_context == NULL) {
            return 0;
        }
        memset(ctx.mode_data.scroll.op_context, 0,
               ctx.mode_data.scroll.op_context_size);

        if(target_win_conf->op->proc.init_context != NULL) {
            if(target_win_conf->op->proc.init_context(
                   ctx.mode_data.scroll.op_context,
                   ctx.mode_data.scroll.op_context_size,
                   &arg) == 0) {
                log_printf(LOG_LEVEL_DEBUG,
                           L"scroll operator: start fail (init_context)\n");
                return 0;
            }
        }

        ctx.mode_data.scroll.op = target_win_conf->op;
    }

    if(wcscmp(ctx.mode_data.scroll.op->name, L"or") != 0) {
        log_printf(LOG_LEVEL_DEBUG, L"scroll operator: ");
        log_print_s_exp(LOG_LEVEL_DEBUG, target_win_conf->args);
        log_printf(LOG_LEVEL_DEBUG, L"\n");
    }

    return 0;
}

static
LRESULT shift_scroll_mode(struct mode_conf *data)
{
    ctx.mode_data.scroll.x_ratio = ctx.mode_data.cur_conf->scroll_mode.x_ratio;
    ctx.mode_data.scroll.y_ratio = ctx.mode_data.cur_conf->scroll_mode.y_ratio;

    log_printf(LOG_LEVEL_DEBUG,
               L"shift scroll mode: %ls\n"
               L"scroll ratio: %8.3lf, %8.3lf\n",
               ctx.mode_data.cur_conf->mode_name,
               ctx.mode_data.scroll.x_ratio, ctx.mode_data.scroll.y_ratio);

    return 0;
}

static
LRESULT end_scroll_mode(struct mode_conf *data)
{
    if(ctx.mode_data.scroll.op != NULL &&
       ctx.mode_data.scroll.op->proc.end_scroll != NULL) {
        ctx.mode_data.scroll.op->proc.end_scroll(
            ctx.mode_data.scroll.op_context);
    }
    ctx.mode_data.scroll.op = NULL;

    if(ctx.mode_data.scroll.op_context != NULL)
        free(ctx.mode_data.scroll.op_context);
    ctx.mode_data.scroll.op_context = NULL;

    if(ctx.mode_data.scroll.class != NULL) free(ctx.mode_data.scroll.class);
    if(ctx.mode_data.scroll.title != NULL) free(ctx.mode_data.scroll.title);
    ctx.mode_data.scroll.class = NULL;
    ctx.mode_data.scroll.title = NULL;

    log_printf(LOG_LEVEL_DEBUG, L"end scroll mode\n");

    return 0;
}

/* WM_MOUSEHOOK_MODECH */
LRESULT scroll_modech(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct mode_conf *data;
    LRESULT ret;

    ctx.mode_data.scroll.start_pt.x = LOWORD(wparam);
    ctx.mode_data.scroll.start_pt.y = HIWORD(wparam);

    data = (struct mode_conf *)lparam;

    switch(data->mode) {
      case MODE_CH_SCROLL:
          if(ctx.mode_data.cur_mode == MODE_CH_SCROLL) {
              ret = shift_scroll_mode(data);
          } else {
              ret = start_scroll_mode(data);
          }
          break;

      case MODE_CH_NORMAL:
          ret = end_scroll_mode(data);
          break;

      default:
          return 0;
    }

    ctx.mode_data.cur_mode = data->mode;

    return ret;
}


/* MODE_MSG_SCROLL */
static
LRESULT scroll_modemsg_scroll(struct mode_conf *data)
{
    if(ctx.mode_data.scroll.op == NULL ||
       ctx.mode_data.scroll.op->proc.scroll == NULL) {
        return 0;
    }

    ctx.mode_data.scroll.op->proc.scroll(ctx.mode_data.scroll.op_context,
                                         ctx.mode_data.scroll.dx,
                                         ctx.mode_data.scroll.dy);

    return 0;
}

/* MODE_MSG_MUL_RATIO */
static
LRESULT scroll_modemsg_mulratio(struct mode_conf *data)
{
    ctx.mode_data.scroll.x_ratio *= data->ratio.x;
    ctx.mode_data.scroll.y_ratio *= data->ratio.y;

    log_printf(LOG_LEVEL_DEBUG,
               L"mul scroll ratio: %8.3lf, %8.3lf\n",
               ctx.mode_data.scroll.x_ratio, ctx.mode_data.scroll.y_ratio);

    return 0;
}

/* MODE_MSG_SET_RATIO */
static
LRESULT scroll_modemsg_setratio(struct mode_conf *data)
{
    ctx.mode_data.scroll.x_ratio = data->ratio.x;
    ctx.mode_data.scroll.y_ratio = data->ratio.y;

    log_printf(LOG_LEVEL_DEBUG,
               L"set scroll ratio: %8.3lf, %8.3lf\n",
               ctx.mode_data.scroll.x_ratio, ctx.mode_data.scroll.y_ratio);

    return 0;
}

/* WM_MOUSEHOOK_MODEMSG */
LRESULT scroll_modemsg(HWND hwnd, UINT msgid, WPARAM wparam, LPARAM lparam)
{
    int x, y;
    struct mode_conf *data;
    MSG msg;

    typedef LRESULT (*mode_proc_t)(struct mode_conf *);
    mode_proc_t proc;

    static struct uint_ptr_pair mode_map[] = {
        {MODE_MSG_SCROLL, scroll_modemsg_scroll},
        {MODE_MSG_MUL_RATIO, scroll_modemsg_mulratio},
        {MODE_MSG_SET_RATIO, scroll_modemsg_setratio},

        {0, NULL}
    };

    x = LOWORD(wparam);
    y = HIWORD(wparam);
    data = (struct mode_conf *)lparam;

    if(data->mode == MODE_MSG_SCROLL) {
        ctx.mode_data.scroll.dx = (x - ctx.mode_data.scroll.start_pt.x) *
                                  ctx.mode_data.scroll.x_ratio;
        ctx.mode_data.scroll.dy = (y - ctx.mode_data.scroll.start_pt.y) *
                                  ctx.mode_data.scroll.y_ratio;

        while(PeekMessage(&msg, hwnd,
                          WM_MOUSEHOOK_MODECH, WM_MOUSEHOOK_MODEMSG,
                          PM_NOREMOVE)) {
            if(msg.message != WM_MOUSEHOOK_MODEMSG) {
                break;
            }

            x = LOWORD(msg.wParam);
            y = HIWORD(msg.wParam);
            data = (struct mode_conf *)msg.lParam;

            if(data->mode != MODE_MSG_SCROLL) {
                break;
            }

            ctx.mode_data.scroll.dx += (x - ctx.mode_data.scroll.start_pt.x) *
                                       ctx.mode_data.scroll.x_ratio;
            ctx.mode_data.scroll.dy += (y - ctx.mode_data.scroll.start_pt.y) *
                                       ctx.mode_data.scroll.y_ratio;

            /* discard */
            PeekMessage(&msg, hwnd,
                        WM_MOUSEHOOK_MODEMSG, WM_MOUSEHOOK_MODEMSG,
                        PM_REMOVE);
        }
    }

    proc = assq_pair(mode_map, data->mode, NULL);
    if(proc) {
        return proc(data);
    }

    return 0;
}
