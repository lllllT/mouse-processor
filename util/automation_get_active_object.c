/*
 * automation_get_active_object.c  -- get_active_object
 *
 */


#define COBJMACROS 1

#include "automation.h"


HRESULT get_active_object(LPCOLESTR progid, IDispatch **dpch)
{
    CLSID clsid;
    IUnknown *unk;
    HRESULT hres;

    hres = CLSIDFromProgID(progid, &clsid);
    if(FAILED(hres)) {
        return hres;
    }

    hres = GetActiveObject(&clsid, NULL, &unk);
    if(FAILED(hres)) {
        return hres;
    }

    hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void **)(void *)dpch);
    IUnknown_Release(unk);
    if(FAILED(hres)) {
        return hres;
    }

    return S_OK;
}
