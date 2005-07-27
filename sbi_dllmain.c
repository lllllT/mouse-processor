/*
 * sbi_dllmain.c  -- entry point of DLL for scrollbar injection
 *
 * $Id: sbi_dllmain.c,v 1.7 2005/07/27 05:54:33 hos Exp $
 *
 */

#include <windows.h>
#include "dllinj.h"
#include "shmem.h"
#include "scroll_op_scrollbar.h"


typedef BOOL (* WINAPI get_scroll_info_proc_t)(HWND, int, LPSCROLLINFO);
static get_scroll_info_proc_t org_get_scroll_info = NULL;

static fake_gsinfo_data_t *gsinfo_data = NULL;
static HANDLE fmap = NULL;

static BOOL init_shmem(void);

__declspec(dllexport) int sbi_dummy; /* for dummy export */


static
BOOL WINAPI fake_get_scroll_info(HWND hwnd, int bar, LPSCROLLINFO si)
{
    BOOL ret;

    ret = org_get_scroll_info(hwnd, bar, si);
    if(ret != FALSE && si != NULL &&
       (gsinfo_data != NULL || init_shmem() == TRUE) &&
       hwnd == gsinfo_data->hwnd &&
       bar == gsinfo_data->bar &&
       (si->fMask & SIF_TRACKPOS) && gsinfo_data->valid) {
        si->nTrackPos = gsinfo_data->track_pos;
    }

    return ret;
}


static
BOOL init_shmem(void)
{
    /* attach shared memory */
    gsinfo_data =
        open_shared_mem(FAKE_GSINFO_SHMEM_NAME, sizeof(fake_gsinfo_data_t),
                        &fmap);
    if(gsinfo_data == NULL) {
        return FALSE;
    }

    return TRUE;
}

static
BOOL init(void)
{
    /* attach shared memory */
    init_shmem();

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

    /* detach shared memory */
    close_shared_mem(gsinfo_data, fmap);
    gsinfo_data = NULL;
    fmap = NULL;

    return TRUE;
}


BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    if(reason == DLL_PROCESS_ATTACH) {
        return init();
    } else if(reason == DLL_PROCESS_DETACH) {
        return finit();
    }

    return TRUE;
}
