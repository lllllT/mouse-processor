/*
 * dllmain.c  -- entry point of DLL
 *
 * $Id: sbi_dllmain.c,v 1.2 2005/02/01 11:28:23 hos Exp $
 *
 */

#include <windows.h>
#include "dllinj.h"


static HINSTANCE dll_instance = NULL;

typedef BOOL (* WINAPI get_scroll_info_proc_t)(HWND, int, LPSCROLLINFO);
static get_scroll_info_proc_t org_get_scroll_info = NULL;


static
BOOL WINAPI fake_get_scroll_info(HWND hwnd, int bar, LPSCROLLINFO si)
{
    return org_get_scroll_info(hwnd, bar, si);
}


static
BOOL init(void)
{
    /* inject */
    org_get_scroll_info = replace_all_imported_proc("USER32.DLL",
                                                    "GetScrollInfo",
                                                    fake_get_scroll_info, 0);
    if(org_get_scroll_info == NULL) {
        return FALSE;
    }

    return TRUE;
}

static
BOOL finit(void)
{
    /* restore */
    replace_all_imported_proc("USER32.DLL",
                              "GetScrollInfo",
                              fake_get_scroll_info, 1);

    return TRUE;
}


BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    if(reason == DLL_PROCESS_ATTACH) {
        dll_instance = instance;
        return init();
    } else if(reason == DLL_PROCESS_DETACH) {
        return finit();
    }

    return TRUE;
}
