/*
 * log.c  -- logging procs
 *
 * $Id: log.c,v 1.13 2005/01/20 08:15:13 hos Exp $
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

    /* handle of log window */
    ctx.log_window = hwnd;

    /* handle of child controls */
    for(i = 0; item_map[i].ptr != NULL; i++) {
        *((HWND *)item_map[i].ptr) =
            GetDlgItem(ctx.log_window, item_map[i].key);
    }

    /* set limit of edit control */
    SendMessageW(log_edit, EM_SETLIMITTEXT, EDIT_TEXT_MAX, 0);

    /* set font of edit control */
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

    /* set position of controls */
    log_dlg_arrannge();

    /* set icon of dialog */
    {
        HICON icon;

        icon = LoadImage(ctx.instance, MAKEINTRESOURCE(ID_ICON_MAIN),
                         IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        SendMessage(ctx.log_window, WM_SETICON, ICON_SMALL, (LPARAM)icon);

        icon = LoadImage(ctx.instance, MAKEINTRESOURCE(ID_ICON_MAIN),
                         IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        SendMessage(ctx.log_window, WM_SETICON, ICON_BIG, (LPARAM)icon);
    }

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
int log_append(const wchar_t *str)
{
    wchar_t buf[256];
    const wchar_t *p;
    DWORD len, buf_len, log_size;

    while(str[0] != 0) {
        p = wcschr(str, L'\n');
        if(p == NULL) {
            p = str + wcslen(str) - 1;
        }

        len = p - str + 1;
        while(len > 0) {
            buf_len = (len > 256 - 3 ? 256 - 3 : len);

            memcpy(buf, str, sizeof(wchar_t) * buf_len);
            if(str[buf_len - 1] == L'\n') {
                buf[buf_len - 1] = L'\r';
                buf[buf_len + 0] = L'\n';
                buf[buf_len + 1] = 0;
            } else {
                buf[buf_len + 0] = 0;
            }

            log_size = SendMessageW(log_edit, WM_GETTEXTLENGTH, 0, 0);
            while(log_size + wcslen(buf) > EDIT_TEXT_MAX) {
                DWORD idx;

                idx = SendMessageW(log_edit, EM_LINEINDEX, 1, 0);
                SendMessageW(log_edit, EM_SETSEL, 0, idx);
                SendMessageW(log_edit, EM_REPLACESEL, FALSE, (LPARAM)L"");

                log_size = SendMessageW(log_edit, WM_GETTEXTLENGTH, 0, 0);
            }

            SendMessageW(log_edit, EM_SETSEL, log_size, log_size);
            SendMessageW(log_edit, EM_REPLACESEL, FALSE, (LPARAM)buf);
            SendMessageW(log_edit, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
            SendMessageW(log_edit, WM_HSCROLL, MAKEWPARAM(SB_LEFT, 0), 0);

            len -= buf_len;
            str += buf_len;
        }
    }

    return 1;
}


int create_logger(void)
{
    if(CreateDialog(ctx.instance, MAKEINTRESOURCE(ID_DLG_LOG),
                    NULL, log_dlg_proc) == NULL) {
        return 0;
    }

    return 1;
}

int destroy_logger(void)
{
    DestroyWindow(ctx.log_window);
    ctx.log_window = NULL;

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
    va_list ap;
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
            return 0;
        }
    }

    va_start(ap, fmt);

    while(1) {
        memset(buf, 0, sizeof(wchar_t) * buf_nch);

        if(_vsnwprintf(buf, buf_nch - 1, fmt, ap) >= 0) {
            break;
        }

        {
            wchar_t *p;
            int nch;

            nch = buf_nch + 256;
            p = (wchar_t *)realloc(buf, sizeof(wchar_t) * nch);
            if(p == NULL) {
                return 0;
            }

            buf = p;
            buf_nch = nch;
        }
    }

    va_end(ap);

    ret = log_append(buf);

    if(level >= LOG_LEVEL_WARNING) {
        show_logger(TRUE);
    }

    return ret;
}

int log_print_s_exp(int level, const s_exp_data_t *data)
{
    int ret;
    unsigned char *str;
    wchar_t *wstr;

    if(level < log_level) {
        return 0;
    }

    str = u8s_write_s_exp(data);
    if(str == NULL) {
        return 0;
    }

    wstr = wcs_dup_from_u8s(str, NULL);
    if(wstr == NULL) {
        free(str);
        return 0;
    }

    ret = log_append(wstr);

    free(str);

    return ret;
}
