/*
 * sbh_dllmain.c  -- entry point of DLL for scrollbar hook
 *
 */

#include <windows.h>
#include "shmem.h"
#include "scroll_op_scrollbar.h"


static fake_gsinfo_data_t *gsinfo_data = NULL;
static HANDLE fmap = NULL;
static HHOOK *hook = NULL;

static WNDPROC org_wnd_proc = NULL;

static BOOL init(void);


static
LRESULT fake_gsi_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT ret;

    ret = CallWindowProc(org_wnd_proc, hwnd, msg, wparam, lparam);
    if(msg == SBM_GETSCROLLINFO &&
       ret != FALSE && lparam != (LPARAM)NULL && gsinfo_data != NULL &&
       hwnd == gsinfo_data->hwnd &&
       (((LPSCROLLINFO)lparam)->fMask & SIF_TRACKPOS)) {
        ((LPSCROLLINFO)lparam)->nTrackPos = gsinfo_data->track_pos;
    }

    return ret;
}

__declspec(dllexport)
LRESULT CALLBACK gsi_call_proc(int code, WPARAM wparam, LPARAM lparam)
{
    CWPSTRUCT *cw = (CWPSTRUCT *)lparam;

    if((gsinfo_data != NULL || init() == TRUE) &&
       cw->message == gsinfo_data->hook_subclass_msg &&
       gsinfo_data->valid &&
       gsinfo_data->bar == SB_CTL) {
        if(org_wnd_proc == NULL) {
            org_wnd_proc =
                (WNDPROC)SetWindowLongPtr(gsinfo_data->hwnd, GWLP_WNDPROC,
                                          (LONG_PTR)fake_gsi_wnd_proc);
        } else {
            SetWindowLongPtr(gsinfo_data->hwnd, GWLP_WNDPROC,
                             (LONG_PTR)org_wnd_proc);
            org_wnd_proc = NULL;
        }
    }

    return CallNextHookEx(*hook, code, wparam, lparam);
}


static
BOOL init(void)
{
    /* attach shared memory */
    gsinfo_data =
        open_shared_mem(FAKE_GSINFO_SHMEM_NAME, sizeof(fake_gsinfo_data_t),
                        &fmap);

    if(gsinfo_data != NULL) {
        hook = &gsinfo_data->hook[gsinfo_data->hook_idx];
    }

    return TRUE;
}

static
BOOL finit(void)
{
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
