/*
 * automation_create_instance_from_progid.c  -- create_instance_from_progid
 *
 */


#define COBJMACROS 1

#include "automation.h"


HRESULT create_instance_from_progid(LPCOLESTR progid, IDispatch **dpch)
{
    CLSID clsid;
    HRESULT hres;

    hres = CLSIDFromProgID(progid, &clsid);
    if(FAILED(hres)) {
        return hres;
    }

    hres = CoCreateInstance(&clsid, NULL, CLSCTX_ALL,
                            &IID_IDispatch, (void **)(void *)dpch);
    if(FAILED(hres)) {
        return hres;
    }

    return S_OK;
}
