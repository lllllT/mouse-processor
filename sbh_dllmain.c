/*
 * sbh_dllmain.c  -- entry point of DLL for scrollbar hook
 *
 * $Id: sbh_dllmain.c,v 1.3 2005/02/03 09:51:13 hos Exp $
 *
 */

#include <windows.h>
#include "shmem.h"
#include "scroll_op_scrollbar.h"

#include <stdio.h>
#define DBG(a1,a2,a3) {FILE *fp = fopen("d:\\dbg.log","a"); fprintf(fp,a1,a2,a3); fclose(fp);}

static fake_gsinfo_data_t *gsinfo_data = NULL;
static HANDLE fmap = NULL;
static HHOOK *hook = NULL;

static WNDPROC org_wnd_proc = NULL;


static
LRESULT fake_gsi_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT ret;

    ret = CallWindowProc(org_wnd_proc, hwnd, msg, wparam, lparam);
    if(msg == SBM_GETSCROLLINFO) {
        DBG("fake: GSI: %p, %x\n",
            hwnd, ((LPSCROLLINFO)lparam)->fMask);
    }
    if(msg == SBM_GETSCROLLINFO &&
       ret != FALSE && lparam != (LPARAM)NULL && gsinfo_data != NULL &&
       hwnd == gsinfo_data->hwnd &&
       (((LPSCROLLINFO)lparam)->fMask & SIF_TRACKPOS)) {
        DBG("fake: replace: %d => %d\n",
            ((LPSCROLLINFO)lparam)->nTrackPos, gsinfo_data->track_pos);
        ((LPSCROLLINFO)lparam)->nTrackPos = gsinfo_data->track_pos;
    }

    return ret;
}

__declspec(dllexport)
LRESULT CALLBACK gsi_call_proc(int code, WPARAM wparam, LPARAM lparam)
{
    CWPSTRUCT *cw = (CWPSTRUCT *)lparam;

    if(gsinfo_data != NULL &&
       cw->message == gsinfo_data->hook_subclass_msg &&
       gsinfo_data->valid &&
       gsinfo_data->bar == SB_CTL) {
        WNDPROC wnd_proc;
        HWND msg_hwnd;
        int track_pos;

        msg_hwnd = GetParent(gsinfo_data->hwnd);

        org_wnd_proc =
            (WNDPROC)SetWindowLongPtr(gsinfo_data->hwnd, GWLP_WNDPROC,
                                      (LONG_PTR)fake_gsi_wnd_proc);
        DBG("subclass set: %p => %p\n", org_wnd_proc, fake_gsi_wnd_proc);

        track_pos = gsinfo_data->track_pos;
        gsinfo_data->track_pos = gsinfo_data->org_pos;
        DBG("send track: %p, %d\n", msg_hwnd, gsinfo_data->track_pos);
        SendMessage(msg_hwnd, gsinfo_data->msg,
                    MAKEWPARAM(SB_THUMBTRACK, gsinfo_data->track_pos),
                    (LPARAM)gsinfo_data->hwnd);

        gsinfo_data->track_pos = track_pos;
        DBG("send track: %p, %d\n", msg_hwnd, gsinfo_data->track_pos);
        SendMessage(msg_hwnd, gsinfo_data->msg,
                    MAKEWPARAM(SB_THUMBTRACK, gsinfo_data->track_pos),
                    (LPARAM)gsinfo_data->hwnd);

        gsinfo_data->track_pos = track_pos;
        DBG("send posit: %p, %d\n", msg_hwnd, gsinfo_data->track_pos);
        SendMessage(msg_hwnd, gsinfo_data->msg,
                    MAKEWPARAM(SB_THUMBPOSITION, gsinfo_data->track_pos),
                    (LPARAM)gsinfo_data->hwnd);

        wnd_proc = SetWindowLongPtr(gsinfo_data->hwnd, GWLP_WNDPROC,
                                    (LONG_PTR)org_wnd_proc);
        DBG("subclass clear: %p => %p\n", wnd_proc, org_wnd_proc);
        wnd_proc = NULL;
    }
    if(cw->message == SBM_GETSCROLLINFO) {
        DBG("call: GSI: %x, %p, ",
            ((LPSCROLLINFO)cw->lParam)->fMask, cw->hwnd);
        DBG("%p, %d\n", gsinfo_data->hwnd, wparam);
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
