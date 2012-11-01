/*
 * window.c  -- window utils
 *
 */

#include "main.h"
#include <psapi.h>


struct enum_notr_data {
    POINT spt;
    HWND target;
};

static
BOOL CALLBACK enum_nontr_window(HWND hwnd, LPARAM lparam)
{
    struct enum_notr_data *data;
    LONG_PTR style;
    HRGN rg;
    RECT rt;
    LRESULT ht;

    data = (struct enum_notr_data *)lparam;

    /* visible? */
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if((style & WS_VISIBLE) != WS_VISIBLE) {
        return TRUE;
    }

    /* inside window rect? */
    if(GetWindowRect(hwnd, &rt) == 0 ||
       PtInRect(&rt, data->spt) == 0) {
        return TRUE;
    }

    /* inside window region? */
    rg = CreateRectRgn(0, 0, 0, 0);
    if(rg == NULL) {
        return TRUE;
    }

    if(GetWindowRgn(hwnd, rg) != ERROR &&
       PtInRegion(rg, data->spt.x - rt.left, data->spt.y - rt.top) == 0) {
        DeleteObject(rg);
        return TRUE;
    }

    DeleteObject(rg);

    /* hittest is not transparent? */
    if(SendMessageTimeout(hwnd, WM_NCHITTEST,
                          0, MAKELPARAM(data->spt.x, data->spt.y),
                          SMTO_ABORTIFHUNG, 1000, &ht) == 0 ||
       ht == HTTRANSPARENT) {
        return TRUE;
    }

    /* ignore tooltip, drop shadow */
    {
        WCHAR buf[256];
        buf[0] = 0;
        GetClassNameW(hwnd, buf, 256);
        if(wcscmp(buf, L"tooltips_class32") == 0 ||
           wcscmp(buf, L"SysShadow") == 0) {
            return TRUE;
        }
    }

    data->target = hwnd;
    return FALSE;
}

HWND get_window_for_mouse_input(POINT spt)
{
    HWND w;

    /*
     * WindowFromPoint() can't use because it does not consider HTTRANSPARENT
     */

    {
        struct enum_notr_data data;

        memset(&data, 0, sizeof(data));
        data.spt = spt;
        EnumWindows(enum_nontr_window, (LPARAM)&data);
        if(data.target == NULL) {
            return NULL;
        }

        w = data.target;
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
            /* maybe non-client area */
            break;
        }

        w = c;
    }

    return w;
}


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
