/*
 * log.c  -- logging procs
 *
 * $Id: log.c,v 1.1 2005/01/14 09:32:37 hos Exp $
 *
 */

#include "main.h"
#include "util.h"
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>
#include <process.h>

#include "resource.h"


static int log_level = LOG_LEVEL_NOTIFY;


static HWND log_window = NULL;
static HWND log_edit = NULL;
static HWND log_close_btn = NULL, log_clear_btn = NULL;
static HWND log_detail_chk = NULL;

static HANDLE log_rpipe_hdl = INVALID_HANDLE_VALUE;
static HANDLE log_wpipe_hdl = INVALID_HANDLE_VALUE;


/* IDOK: BN_CLICK */
static
INT_PTR log_close_clicked(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    show_logger(FALSE);

    return TRUE;
}

/* ID_BTN_CLEAR: BN_CLICK */
static
INT_PTR log_clear_clicked(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    SetWindowTextW(log_edit, L"");

    return TRUE;
}

/* ID_CHK_DETAIL: BN_CLICK */
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

/* WM_SIZE */
static
INT_PTR log_dlg_size(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    RECT rt, brt, crt;

    GetClientRect(log_window, &rt);

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

    RedrawWindow(log_window, NULL, NULL,
                 RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);

    return TRUE;
}

/* WM_CLOSE */
static
INT_PTR log_dlg_close(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    show_logger(FALSE);

    return TRUE;
}


typedef INT_PTR (* dlg_proc_t)(HWND, UINT, WPARAM, LPARAM);

static
INT_PTR log_dlg_command(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    dlg_proc_t proc;

    static struct uint_ptr_pair msg_map[] = {
        {MAKEWPARAM(IDOK, BN_CLICKED), log_close_clicked},
        {MAKEWPARAM(IDCANCEL, 0), log_close_clicked},
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

static
INT_PTR CALLBACK log_dlg_proc(HWND hwnd, UINT msg,
                              WPARAM wparam, LPARAM lparam)
{
    dlg_proc_t proc;

    static struct uint_ptr_pair msg_map[] = {
        {WM_COMMAND, log_dlg_command},
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
    unsigned char buf[512], *buf_start;
    const unsigned char *rest;
    wchar_t *wbuf;
    DWORD buf_offset, rest_size, read_size;
    DWORD log_size;

    buf_offset = 0;
    while(1) {
        buf_start = buf + buf_offset;
        rest_size = sizeof(buf) - buf_offset - 1;

        memset(buf_start, 0, rest_size + 1);

        if(ReadFile(log_rpipe_hdl,
                    buf_start, rest_size, &read_size, NULL) == 0) {
            break;
        }

        if(read_size <= 0) {
            break;
        }

        buf_start[read_size] = 0;
        wbuf = wcs_dup_from_u8s(buf, &rest);
        if(wbuf == NULL) {
            break;
        }

        log_size = GetWindowTextLengthW(log_edit);
        SendMessageW(log_edit, EM_SETSEL, log_size, log_size);
        SendMessageW(log_edit, EM_REPLACESEL, FALSE, (LPARAM)wbuf);

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


int create_logger(void)
{
    log_window = CreateDialog(ctx.instance, MAKEINTRESOURCE(ID_DLG_LOG),
                              ctx.main_window, log_dlg_proc);
    if(log_window == NULL) {
        return 0;
    }

    {
        int i;
        static struct uint_ptr_pair item_map[] = {
            {ID_EDIT_LOG, &log_edit},
            {IDOK, &log_close_btn},
            {ID_BTN_LOG_CLEAR, &log_clear_btn},
            {ID_CHK_LOG_DETAIL, &log_detail_chk},

            {0, NULL}
        };

        for(i = 0; item_map[i].ptr != NULL; i++) {
            *((HWND *)item_map[i].ptr) =
                GetDlgItem(log_window, item_map[i].key);
            if(*((HWND *)item_map[i].ptr) == NULL) {
                return 0;
            }
        }
    }

    if(start_logger() == 0) {
        return 0;
    }

    return 1;
}

int destroy_logger(void)
{
    DestroyWindow(log_window);
    CloseHandle(log_wpipe_hdl);

    return 1;
}

int show_logger(BOOL show)
{
    return ShowWindow(log_window, (show ? SW_SHOW : SW_HIDE));
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
            p = (wchar_t *)realloc(buf, sizeof(wchar_t) * buf_nch);
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

            buf_size = strlen(u8s_buf) + 1;

            ret = 0;
            p = u8s_buf;
            while(1) {
                if(WriteFile(log_wpipe_hdl,
                             p, buf_size, &wsize, NULL) == 0) {
                    break;
                }

                if(wsize == 0) {
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
