/*
 * log.c  -- logging procs
 *
 * $Id: log.c,v 1.9 2005/01/18 09:36:40 hos Exp $
 *
 */

#include "main.h"
#include "util.h"
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>
#include <process.h>

#include "resource.h"


#define EDIT_TEXT_MAX 0x00ffffff

static int log_level = LOG_LEVEL_NOTIFY;


static HWND log_edit = NULL;
static HWND log_close_btn = NULL, log_clear_btn = NULL;
static HWND log_detail_chk = NULL;

static HFONT log_edit_font = NULL;

static HANDLE log_rpipe_hdl = INVALID_HANDLE_VALUE;
static HANDLE log_wpipe_hdl = INVALID_HANDLE_VALUE;


static
void log_dlg_arrannge(void)
{
    RECT rt, brt, crt;

    GetClientRect(ctx.log_window, &rt);

    GetClientRect(log_clear_btn, &brt);
    MoveWindow(log_clear_btn,
               3, rt.bottom - (brt.bottom + 3),
               brt.right, brt.bottom,
               FALSE);

    GetClientRect(log_close_btn, &brt);
    MoveWindow(log_close_btn,
               rt.right - (brt.right + 3), rt.bottom - (brt.bottom + 3),
               brt.right, brt.bottom,
               FALSE);

    GetClientRect(log_detail_chk, &crt);
    MoveWindow(log_detail_chk,
               rt.right - (crt.right + 3 + brt.right + 3),
               rt.bottom - (crt.bottom + 3),
               crt.right, crt.bottom,
               FALSE);

    MoveWindow(log_edit,
               0, 0,
               rt.right, rt.bottom - (3 + brt.bottom + 3),
               FALSE);

    RedrawWindow(ctx.log_window, NULL, NULL,
                 RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
}


/* IDOK: BN_CLICKED */
static
INT_PTR log_close_clicked(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    show_logger(FALSE);

    return TRUE;
}

/* ID_BTN_CLEAR: BN_CLICKED */
static
INT_PTR log_clear_clicked(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    SetWindowTextW(log_edit, L"");

    return TRUE;
}

/* ID_CHK_DETAIL: BN_CLICKED */
static
INT_PTR log_detail_clicked(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if(SendMessage(log_detail_chk, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        log_level = LOG_LEVEL_DEBUG;
    } else {
        log_level = LOG_LEVEL_NOTIFY;
    }

    return TRUE;
}

/* WM_INITDIALOG */
static
INT_PTR log_dlg_init(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int i;
    static struct uint_ptr_pair item_map[] = {
        {ID_EDIT_LOG, &log_edit},
        {IDOK, &log_close_btn},
        {ID_BTN_LOG_CLEAR, &log_clear_btn},
        {ID_CHK_LOG_DETAIL, &log_detail_chk},

        {0, NULL}
    };

    ctx.log_window = hwnd;

    for(i = 0; item_map[i].ptr != NULL; i++) {
        *((HWND *)item_map[i].ptr) =
            GetDlgItem(ctx.log_window, item_map[i].key);
    }

    SendMessageW(log_edit, EM_SETLIMITTEXT, EDIT_TEXT_MAX, 0);

    {
        TCHAR name[64], size[64], charset[64];

        memset(name, 0, sizeof(name));
        memset(size, 0, sizeof(size));
        memset(charset, 0, sizeof(charset));

        LoadString(ctx.instance, ID_STR_LOG_EDIT_FONT, name, 64);
        LoadString(ctx.instance, ID_STR_LOG_EDIT_FONT_SIZE, size, 64);
        LoadString(ctx.instance, ID_STR_LOG_EDIT_FONT_CS, charset, 64);

        log_edit_font = CreateFont(_ttoi(size), 0, 0, 0,
                                   FW_NORMAL, FALSE, FALSE, FALSE,
                                   _ttoi(charset),
                                   OUT_STRING_PRECIS, CLIP_STROKE_PRECIS,
                                   DEFAULT_QUALITY,
                                   FIXED_PITCH | FF_MODERN,
                                   name);
        if(log_edit_font != NULL) {
            SendMessageW(log_edit, WM_SETFONT, (WPARAM)log_edit_font, FALSE);
        }
    }

    log_dlg_arrannge();

    return TRUE;
}

/* WM_DESTROY */
static
INT_PTR log_dlg_destroy(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    DeleteObject(log_edit_font);

    return TRUE;
}

/* WM_SIZE */
static
INT_PTR log_dlg_size(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    log_dlg_arrannge();

    return TRUE;
}

/* WM_CLOSE */
static
INT_PTR log_dlg_close(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    show_logger(FALSE);

    return TRUE;
}

/* SC_MINIMIZE */
static
INT_PTR log_dlg_minimize(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    show_logger(FALSE);

    return TRUE;
}


typedef INT_PTR (* dlg_proc_t)(HWND, UINT, WPARAM, LPARAM);

/* WM_COMMAND */
static
INT_PTR log_dlg_command(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    dlg_proc_t proc;

    static struct uint_ptr_pair msg_map[] = {
        {MAKEWPARAM(IDOK, BN_CLICKED), log_close_clicked},
        {MAKEWPARAM(IDCANCEL, BN_CLICKED), log_close_clicked},
        {MAKEWPARAM(ID_BTN_LOG_CLEAR, BN_CLICKED), log_clear_clicked},
        {MAKEWPARAM(ID_CHK_LOG_DETAIL, BN_CLICKED), log_detail_clicked},

        {0, NULL}
    };

    proc = (dlg_proc_t)assq_pair(msg_map, wparam, NULL);
    if(proc == NULL) {
        return FALSE;
    }

    return proc(hwnd, msg, wparam, lparam);
}

/* WM_SYSCOMMAND */
static
INT_PTR log_dlg_syscommand(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    dlg_proc_t proc;

    static struct uint_ptr_pair msg_map[] = {
        {SC_MINIMIZE, log_dlg_minimize},

        {0, NULL}
    };

    proc = (dlg_proc_t)assq_pair(msg_map, wparam, NULL);
    if(proc == NULL) {
        return FALSE;
    }

    return proc(hwnd, msg, wparam, lparam);
}

static
INT_PTR CALLBACK log_dlg_proc(HWND hwnd, UINT msg,
                              WPARAM wparam, LPARAM lparam)
{
    dlg_proc_t proc;

    static struct uint_ptr_pair msg_map[] = {
        {WM_INITDIALOG, log_dlg_init},
        {WM_DESTROY, log_dlg_destroy},
        {WM_COMMAND, log_dlg_command},
        {WM_SYSCOMMAND, log_dlg_syscommand},
        {WM_SIZE, log_dlg_size},
        {WM_CLOSE, log_dlg_close},

        {0, NULL}
    };

    proc = (dlg_proc_t)assq_pair(msg_map, msg, NULL);
    if(proc == NULL) {
        return FALSE;
    }

    return proc(hwnd, msg, wparam, lparam);
}


static
void __cdecl log_thread(void *arg)
{
    unsigned char buf[512], text[512], *buf_start, *pp;
    const unsigned char *rest;
    wchar_t *wbuf;
    DWORD buf_offset, rest_size, read_size, plen;
    DWORD log_size;

    buf_offset = 0;
    while(1) {
        buf_start = buf + buf_offset;
        rest_size = sizeof(buf) - buf_offset - 2;

        memset(buf_start, 0, rest_size + 2);

        if(ReadFile(log_rpipe_hdl,
                    buf_start, rest_size, &read_size, NULL) == 0) {
            break;
        }

        if(read_size <= 0) {
            break;
        }

        buf_start[read_size] = 0;

        plen = 0;
        rest = "";
        while(1) {
            strcpy(text, buf + plen);

            pp = strchr(text, '\n');
            if(pp != NULL) {
                pp[0] = '\r';
                pp[1] = '\n';
                pp[2] = 0;

                plen += (pp - text) + 1;
            }

            wbuf = wcs_dup_from_u8s(text, &rest);
            if(wbuf == NULL) {
                break;
            }

            log_size = SendMessageW(log_edit, WM_GETTEXTLENGTH, 0, 0);
            while(log_size + wcslen(wbuf) > EDIT_TEXT_MAX) {
                DWORD idx;

                idx = SendMessageW(log_edit, EM_LINEINDEX, 1, 0);
                SendMessageW(log_edit, EM_SETSEL, 0, idx);
                SendMessageW(log_edit, EM_REPLACESEL, FALSE, (LPARAM)L"");

                log_size = SendMessageW(log_edit, WM_GETTEXTLENGTH, 0, 0);
            }

            SendMessageW(log_edit, EM_SETSEL, log_size, log_size);
            SendMessageW(log_edit, EM_REPLACESEL, FALSE, (LPARAM)wbuf);
            SendMessageW(log_edit, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
            SendMessageW(log_edit, WM_HSCROLL, MAKEWPARAM(SB_LEFT, 0), 0);

            free(wbuf);

            if(pp == NULL) {
                break;
            }
        }

        buf_offset = strlen(rest);
        memmove(buf, rest, buf_offset);
    }

    CloseHandle(log_rpipe_hdl);
}

static
int start_logger(void)
{
    HANDLE pin = INVALID_HANDLE_VALUE, pout = INVALID_HANDLE_VALUE;

    if(CreatePipe(&pin, &pout, NULL, 0) == 0) {
        goto fail_end;
    }

    log_rpipe_hdl = pin;
    log_wpipe_hdl = pout;

    if((long)_beginthread(log_thread, 0, NULL) == -1) {
        goto fail_end;
    }

    return 1;

  fail_end:
    if(pin != INVALID_HANDLE_VALUE) CloseHandle(pin);
    if(pout != INVALID_HANDLE_VALUE) CloseHandle(pout);
    log_rpipe_hdl = INVALID_HANDLE_VALUE;
    log_wpipe_hdl = INVALID_HANDLE_VALUE;

    return 0;
}


struct log_dlg_data {
    int ret;
    DWORD last_error;
    HANDLE start_evt;
};

static
void __cdecl log_dlg_thread(void *arg)
{
    struct log_dlg_data *data = (struct log_dlg_data *)arg;
    MSG msg;
    int ret;

    if(CreateDialog(ctx.instance, MAKEINTRESOURCE(ID_DLG_LOG),
                    NULL, log_dlg_proc) == NULL) {
        data->ret = 0;
    } else {
        data->ret = 1;
    }
    data->last_error = GetLastError();
    SetEvent(data->start_evt);

    while(ctx.log_window != NULL) {
        ret = GetMessage(&msg, NULL, 0, 0);
        if(ret <= 0) {
            break;
        }

        ret = IsDialogMessage(ctx.log_window, &msg);
        if(ret != 0) {
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    _endthread();
}

static
int start_log_dlg(void)
{
    struct log_dlg_data data;
    int ret;

    memset(&data, 0, sizeof(data));

    data.start_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(data.start_evt == NULL) {
        return 0;
    }

    ret = (long)_beginthread(log_dlg_thread, 0, &data);
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


int create_logger(void)
{
    if(start_log_dlg() == 0) {
        return 0;
    }

    if(start_logger() == 0) {
        return 0;
    }

    return 1;
}

int destroy_logger(void)
{
    DestroyWindow(ctx.log_window);
    ctx.log_window = NULL;

    CloseHandle(log_wpipe_hdl);

    return 1;
}

int show_logger(BOOL show)
{
    ShowWindow(ctx.log_window, (show ? SW_SHOW : SW_HIDE));
    if(show) {
        SetForegroundWindow(ctx.log_window);
    }

    return 1;
}

int log_printf(int level, const wchar_t *fmt, ...)
{
    static wchar_t *buf = NULL;
    static int buf_nch = -1;
    int ret;

    if(level < log_level) {
        return 0;
    }

    if(buf == NULL) {
        buf_nch = 256;
        buf = (wchar_t *)malloc(sizeof(wchar_t) * buf_nch);
        if(buf == NULL) {
            return -1;
        }
    }

    va_list ap;

    va_start(ap, fmt);

    while(1) {
        memset(buf, 0, sizeof(wchar_t) * buf_nch);

        if(_vsnwprintf(buf, buf_nch - 1, fmt, ap) < 0) {
            wchar_t *p;
            int nch;

            nch = buf_nch + 256;
            p = (wchar_t *)realloc(buf, sizeof(wchar_t) * nch);
            if(p == NULL) {
                return -1;
            }

            buf = p;
            buf_nch = nch;
            continue;
        }

        {
            unsigned char *u8s_buf, *p;
            DWORD buf_size, wsize;

            u8s_buf = u8s_dup_from_wcs(buf);
            if(u8s_buf == NULL) {
                return -1;
            }

            buf_size = strlen(u8s_buf);

            ret = 0;
            p = u8s_buf;
            while(1) {
                if(WriteFile(log_wpipe_hdl, p, buf_size, &wsize, NULL) == 0) {
                    break;
                }

                if(wsize <= 0) {
                    break;
                }

                buf_size -= wsize;
                p += wsize;
                ret += wsize;

                if(buf_size <= 0) {
                    break;
                }
            }

            free(u8s_buf);
        }

        break;
    }

    va_end(ap);

    if(level >= LOG_LEVEL_WARNING) {
        show_logger(TRUE);
    }

    return ret;
}

int log_print_s_exp(int level, const s_exp_data_t *data)
{
    int ret;
    unsigned char *str, *p;
    DWORD len, ws;

    if(level < log_level) {
        return 0;
    }

    str = u8s_write_s_exp(data);
    if(str == NULL) {
        return -1;
    }

    len = strlen(str);

    p = str;
    ret = 0;
    while(1) {
        if(WriteFile(log_wpipe_hdl, p, len, &ws, NULL) == 0) {
            break;
        }

        if(ws <= 0) {
            break;
        }

        len -= ws;
        p += ws;
        ret += ws;

        if(len <= 0) {
            break;
        }
    }

    return ret;
}
