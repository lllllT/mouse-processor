/*
 * sbh_dllmain.c  -- entry point of DLL for scrollbar hook
 *
 * $Id: sbh_dllmain.c,v 1.1 2005/02/02 10:03:52 hos Exp $
 *
 */

#include <windows.h>
#include "shmem.h"
#include "scroll_op_scrollbar.h"

#include <stdio.h>
#define DBG(a1,a2,a3) {FILE *fp = fopen("d:\\dbg.log", "a"); fprintf(fp,a1,a2,a3); fclose(fp);}

static fake_gsinfo_data_t *gsinfo_data = NULL;
static HANDLE fmap = NULL;
static HHOOK *hook = NULL;


__declspec(dllexport)
LRESULT CALLBACK gsi_call_proc(int code, WPARAM wparam, LPARAM lparam)
{
    CWPSTRUCT *cw = (CWPSTRUCT *)lparam;

    if(cw->message == gsinfo_data->hook_subclass_msg) {
        /* todo: subclass target window */
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
        DBG("hook: %p\n", hook, 0);
    }

    DBG("cw: init success: %p, %p\n", gsinfo_data, fmap);
    return TRUE;
}

static
BOOL finit(void)
{
    /* detach shared memory */
    close_shared_mem(gsinfo_data, fmap);
    gsinfo_data = NULL;
    fmap = NULL;

    DBG("cw: finit\n", 0, 0);
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
