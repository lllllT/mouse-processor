/*
 * main.c  -- main part of mouse-processor
 *
 * $Id: main.c,v 1.27 2005/01/21 04:38:13 hos Exp $
 *
 */

#include "main.h"
#include "util.h"

#include "resource.h"

#define LR_VGACOLOR 0x0080


#define WM_TASKTRAY (WM_APP + 1)

typedef LRESULT (* msg_proc_t)(HWND, UINT, WPARAM, LPARAM);


struct app_context ctx;


const LPTSTR window_class_name = _T("mouse-processor");
const LPTSTR window_title_name = _T("mouse-processor");

const int notify_icon_id = 1;

static int taskbar_created_message = 0;

static HMENU popup_menu = NULL;

static int pause = 0;


static
int is_winxp_or_later(void)
{
    OSVERSIONINFOEX osv;
    DWORDLONG cond;

    memset(&osv, 0, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(osv);
    osv.dwMajorVersion = 5;
    osv.dwMinorVersion = 1;

    cond = 0;
    cond = VerSetConditionMask(cond, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);

    return VerifyVersionInfo(&osv, VER_MAJORVERSION | VER_MINORVERSION, cond);
}

/* tasktray icon */
static
int set_tasktray_icon(HWND hwnd, int msg)
{
    NOTIFYICONDATA ni;
    HICON icon;

    icon = LoadImage(ctx.instance, MAKEINTRESOURCE(ID_ICON_MAIN),
                     IMAGE_ICON, 16, 16,
                     (is_winxp_or_later() ? LR_DEFAULTCOLOR : LR_VGACOLOR));
    if(icon == NULL) {
        return 0;
    }

    memset(&ni, 0, sizeof(ni));
    ni.cbSize = sizeof(ni);
    ni.hWnd = hwnd;
    ni.uID = notify_icon_id;
    ni.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    ni.hIcon = icon;
    ni.uCallbackMessage = WM_TASKTRAY;
    _tcscpy(ni.szTip, window_title_name);

    return Shell_NotifyIcon(msg, &ni);
}


/* menu item state */
static
int set_pause_menu_item(int state)
{
    MENUITEMINFO mi;

    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_STATE;
    mi.fState = state;

    return SetMenuItemInfo(popup_menu, ID_MENU_PAUSE, FALSE, &mi);
}


static
void error_message(LPWSTR msg)
{
    if(ctx.log_window != NULL) {
        log_printf(LOG_LEVEL_ERROR, L"Error: %ls\n", msg);
    } else {
        MessageBoxW(NULL, msg, window_title_name, MB_OK | MB_ICONERROR);
    }
}

static
void error_message_id(const char *msg, DWORD id, BOOL is_hex)
{
    LPWSTR buf1 = NULL, buf2 = NULL;

    if(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, id, 0, (LPWSTR)(void *)&buf1, 0, NULL) != 0) {
        buf2 = (WCHAR *)
               malloc(sizeof(WCHAR) * (strlen(msg) + 2 + wcslen(buf1) + 1));
        if(buf2 == NULL) {
            goto end;
        }

        wsprintfW(buf2, L"%hs: %ls", msg, buf1);
    } else {
        buf2 = (WCHAR *)malloc(sizeof(WCHAR) * (strlen(msg) + 32));
        if(buf2 == NULL) {
            goto end;
        }

        wsprintfW(buf2, (is_hex ? L"%hs: 0x%08X" : L"%hs: %d"), msg, id);
    }

    error_message(buf2);

  end:
    LocalFree(buf1);
    free(buf2);
}

void error_message_le(const char *msg)
{
    error_message_id(msg, GetLastError(), FALSE);
}

void error_message_hr(const char *msg, HRESULT hr)
{
    if(HRESULT_FACILITY(hr) == FACILITY_WINDOWS) {
        error_message_id(msg, HRESULT_CODE(hr), FALSE);
    } else {
        error_message_id(msg, hr, TRUE);
    }
}


static
LRESULT default_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/* menu item: reload */
static
LRESULT menu_reload(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    load_setting(ctx.app_conf.conf_file, FALSE);

    return 0;
}

/* menu item: pause */
static
LRESULT menu_pause(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int ret;

    /* toggle pasue state */
    pause = !pause;

    set_pause_menu_item(pause ? MFS_CHECKED : MFS_UNCHECKED);

    if(pause) {
        ret = clear_hook();
        if(ret == 0) {
            error_message_le("clear_hook() failed");
        }
    } else {
        ret = set_hook();
        if(ret == 0) {
            error_message_le("set_hook() failed");
        }
    }

    return 0;
}

/* menu item: log */
static
LRESULT menu_log(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    show_logger(TRUE);

    return 0;
}

/* menu item: exit */
static
LRESULT menu_exit(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    DestroyWindow(hwnd);

    return 0;
}

/* tray: WM_RBUTTONUP */
static
LRESULT tray_rbutton_up(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    POINT pos;
    int ret;

    GetCursorPos(&pos);

    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    ret = TrackPopupMenu(popup_menu, TPM_LEFTALIGN | TPM_BOTTOMALIGN,
                         pos.x, pos.y, 0, hwnd, NULL);
    if(ret == 0) {
        error_message_le("TrackPopupMenu() failed");
    }

    return 0;
}

/* tray: WM_LBUTTONDBLCLK */
static
LRESULT tray_lbutton_dblclick(HWND hwnd, UINT msg,
                              WPARAM wparam, LPARAM lparam)
{
    show_logger(TRUE);

    return 0;
}

/* WM_CREATE */
static
LRESULT main_create(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int ret;

    {
        HMENU menu;

        menu = LoadMenu(ctx.instance, MAKEINTRESOURCE(ID_MENU_TRAY));
        if(menu == NULL) {
            error_message_le("LoadMenu() failed");
            return -1;
        }

        popup_menu = GetSubMenu(menu, 0);
        if(popup_menu == 0) {
            error_message_le("GetSubMenu() failed");
            return -1;
        }

        ret = SetMenuDefaultItem(popup_menu, ID_MENU_LOG, FALSE);
        if(ret == 0) {
            error_message_le("SetMenuDefaultItem() failed");
            return -1;
        }
    }

    ret = set_tasktray_icon(hwnd, NIM_ADD);
    if(ret == 0) {
        error_message_le("set_tasktray_icon() failed");
        return -1;
    }

    ret = set_hook();
    if(ret == 0) {
        error_message_le("set_hook() failed");
        return -1;
    }

    return 0;
}

/* WM_DESTROY */
static
LRESULT main_destroy(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    set_tasktray_icon(hwnd, NIM_DELETE);
    clear_hook();

    PostQuitMessage(0);

    return 0;
}

/* "TaskbarCreated" */
static
LRESULT main_taskbar_created(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int ret;

    ret = set_tasktray_icon(hwnd, NIM_ADD);
    if(ret == 0) {
        error_message_le("set_tasktray_icon() failed");
    }

    return 0;
}

/* WM_TASKTRAY */
static
LRESULT main_tasktray(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static struct uint_ptr_pair msg_map[] = {
        {WM_RBUTTONUP, tray_rbutton_up},
        {WM_LBUTTONDBLCLK, tray_lbutton_dblclick},

        {0, NULL}
    };
    msg_proc_t proc;

    proc = (msg_proc_t)assq_pair(msg_map, lparam, NULL);

    if(proc) {
        return proc(hwnd, msg, wparam, lparam);
    } else {
        return 0;
    }
}

/* WM_COMMAND */
static
LRESULT main_command(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static struct uint_ptr_pair msg_map[] = {
        {MAKEWPARAM(ID_MENU_PAUSE, 0), menu_pause},
        {MAKEWPARAM(ID_MENU_EXIT, 0), menu_exit},
        {MAKEWPARAM(ID_MENU_LOG, 0), menu_log},
        {MAKEWPARAM(ID_MENU_RELOAD, 0), menu_reload},

        {0, NULL}
    };
    msg_proc_t proc;

    proc = (msg_proc_t)assq_pair(msg_map, wparam, default_proc);

    return proc(hwnd, msg, wparam, lparam);
}

/* window procedure */
static
LRESULT CALLBACK main_window_proc(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam)
{
    static struct uint_ptr_pair msg_map[] = {
        {0, main_taskbar_created},
        {WM_CREATE, main_create},
        {WM_DESTROY, main_destroy},
        {WM_COMMAND, main_command},
        {WM_TASKTRAY, main_tasktray},

        {WM_MOUSEHOOK_MODECH, scroll_modech},
        {WM_MOUSEHOOK_MODEMSG, scroll_modemsg},

        {0, NULL}
    };
    msg_proc_t proc;

    msg_map[0].key = taskbar_created_message;

    proc = (msg_proc_t)assq_pair(msg_map, msg, default_proc);

    return proc(hwnd, msg, wparam, lparam);
}


static
HWND create_main_window(void)
{
    {
        WNDCLASSEX wc;
        int ret;

        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = 0;
        wc.lpfnWndProc = main_window_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = ctx.instance;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = window_class_name;
        wc.hIconSm = NULL;

        ret = RegisterClassEx(&wc);
        if(ret == 0) {
            return NULL;
        }        
    }


    {
        HWND wnd;

        wnd = CreateWindow(window_class_name, window_title_name,
                           0, 0, 0, 0, 0,
                           HWND_MESSAGE, NULL, ctx.instance, NULL);

        return wnd;
    }
}


static
int message_loop(void)
{
    MSG msg;
    int ret;

    while(1) {
        ret = GetMessage(&msg, NULL, 0, 0);
        if(ret < 0) {
            error_message_le("GetMessage() failed");
            return 1;
        }

        if(ret == 0) {
            return msg.wParam;
        }

        if(ctx.log_window != NULL &&
           IsDialogMessage(ctx.log_window, &msg) != 0) {
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;                   /* not reach */
}

/* entry point */
int main(int ac, char **av)
{
    int ret;

    memset(&ctx, 0, sizeof(ctx));

    /* instance handle of this module */
    {
        HMODULE module;

        module = GetModuleHandle(NULL);
        if(module == NULL) {
            error_message_le("GetModuleHandle() failed");
            return 1;
        }

        ctx.instance = (HINSTANCE)module;
    }

    /* message id of taskbar created notify */
    taskbar_created_message = RegisterWindowMessage(_T("TaskbarCreated"));
    if(taskbar_created_message == 0) {
        error_message_le("RegisterWindowMessage() failed");
        return 1;
    }

    /* initialize COM */
    {
        HRESULT hres;

        hres = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if(FAILED(hres)) {
            error_message_hr("CoInitializeEx() failed", hres);
            return 1;
        }

        hres = init_regexp();
        if(FAILED(hres)) {
            error_message_hr("init_regexp() failed", hres);
            return 1;
        }
    }

    /* create main message window */
    ctx.main_window = create_main_window();
    if(ctx.main_window == NULL) {
        error_message_le("create_main_window() failed");
        return 1;
    }

    /* logger */
    ret = create_logger();
    if(ret == 0) {
        error_message_le("create_logger() failed");
        return 1;
    }

    /* command line option */
    {
        LPWSTR *avw, file;
        int acw;

        avw = CommandLineToArgvW(GetCommandLineW(), &acw);
        if(avw != NULL && acw >= 2) {
            file = avw[1];
        } else {
            file = NULL;
        }

        /* load setting file */
        load_setting(file, TRUE);
    }

    /* start message */
    log_printf(LOG_LEVEL_NOTIFY, L"\nmouse-processor started\n");

    /* main message loop */
    ret = message_loop();

    /* uninitialize COM */
    CoUninitialize();

    /* end logger */
    destroy_logger();

    return ret;
}
