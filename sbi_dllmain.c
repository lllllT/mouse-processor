/*
 * sbi_dllmain.c  -- entry point of DLL for scrollbar injection
 *
 * $Id: sbi_dllmain.c,v 1.4 2005/02/02 10:03:52 hos Exp $
 *
 */

#include <windows.h>
#include "dllinj.h"
#include "shmem.h"
#include "scroll_op_scrollbar.h"

#include <stdio.h>
#define DBG(a1,a2,a3) {FILE *fp = fopen("d:\\dbg.log", "a"); fprintf(fp,a1,a2,a3); fclose(fp);}

typedef BOOL (* WINAPI get_scroll_info_proc_t)(HWND, int, LPSCROLLINFO);
static get_scroll_info_proc_t org_get_scroll_info = NULL;

static fake_gsinfo_data_t *gsinfo_data = NULL;
static HANDLE fmap = NULL;


static
BOOL WINAPI fake_get_scroll_info(HWND hwnd, int bar, LPSCROLLINFO si)
{
    BOOL ret;

    ret = org_get_scroll_info(hwnd, bar, si);
    DBG("fke: %d, %p, ", GetCurrentThreadId(), hwnd);
    DBG("%p, %d, ", gsinfo_data->hwnd, ret);
    DBG("%d, 0x%X\n", gsinfo_data->valid, si->fMask);
    if(ret != FALSE && si != NULL && gsinfo_data != NULL &&
       hwnd == gsinfo_data->hwnd &&
       bar == gsinfo_data->bar &&
       (si->fMask & SIF_TRACKPOS) && gsinfo_data->valid) {
        DBG("fke: track replace: %d -> %d\n",
            si->nTrackPos, gsinfo_data->track_pos);
        si->nTrackPos = gsinfo_data->track_pos;
    }

    return ret;
}


static
BOOL init(void)
{
    /* attach shared memory */
    gsinfo_data =
        open_shared_mem(FAKE_GSINFO_SHMEM_NAME, sizeof(fake_gsinfo_data_t),
                        &fmap);
    if(gsinfo_data == NULL) {
        return FALSE;
    }

    /* inject */
    org_get_scroll_info = replace_all_imported_proc("USER32.DLL",
                                                    "GetScrollInfo",
                                                    fake_get_scroll_info, 0);
    if(org_get_scroll_info == NULL) {
        return FALSE;
    }

    DBG("fke: init success: %p, %p\n", gsinfo_data, fmap);
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

    DBG("fke: finit\n", 0, 0);
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
