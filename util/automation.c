/*
 * automation.c  -- COM automation helper
 *
 * $Id: automation.c,v 1.1 2005/01/05 07:46:31 hos Exp $
 *
 */


#include <objbase.h>
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


HRESULT invoke(IDispatch *dpch, DISPID dpid, WORD flags,
               DISPPARAMS *params, VARIANT *result)
{
    return IDispatch_Invoke(dpch, dpid, &IID_NULL, LOCALE_USER_DEFAULT, flags,
                            params, result, NULL, NULL);
}

HRESULT get_dpid(IDispatch *dpch, LPOLESTR name, DISPID *dpid)
{
    return IDispatch_GetIDsOfNames(dpch, &IID_NULL, &name, 1,
                                   LOCALE_USER_DEFAULT, dpid);
}


HRESULT call_method(IDispatch *dpch, DISPID dpid,
                    VARIANTARG *args, unsigned int nargs, VARIANT *result)
{
    DISPPARAMS params;

    params.rgvarg = args;
    params.cArgs = nargs;
    params.rgdispidNamedArgs = NULL;
    params.cNamedArgs = 0;

    return invoke(dpch, dpid, DISPATCH_METHOD, &params, result);
}

HRESULT get_property(IDispatch *dpch, DISPID dpid, VARIANT *result)
{
    DISPPARAMS params;

    params.rgvarg = NULL;
    params.cArgs = 0;
    params.rgdispidNamedArgs = NULL;
    params.cNamedArgs = 0;

    return invoke(dpch, dpid, DISPATCH_PROPERTYGET | DISPATCH_METHOD,
                  &params, result);
}

HRESULT put_property(IDispatch *dpch, DISPID dpid, VARIANTARG *val)
{
    DISPPARAMS params;
    DISPID arg_dpid;

    params.rgvarg = val;
    params.cArgs = 1;
    arg_dpid = DISPID_PROPERTYPUT;
    params.rgdispidNamedArgs = &arg_dpid;
    params.cNamedArgs = 1;

    return invoke(dpch, dpid, DISPATCH_PROPERTYPUT, &params, NULL);
}


HRESULT call_method_s(IDispatch *dpch, LPOLESTR name,
                      VARIANTARG *args, unsigned int nargs, VARIANT *result)
{
    DISPID dpid;
    HRESULT hres;

    hres = get_dpid(dpch, name, &dpid);
    if(FAILED(hres)) {
        return hres;
    }

    return call_method(dpch, dpid, args, nargs, result);
}


HRESULT get_property_s(IDispatch *dpch, LPOLESTR name, VARIANT *result)
{
    DISPID dpid;
    HRESULT hres;

    hres = get_dpid(dpch, name, &dpid);
    if(FAILED(hres)) {
        return hres;
    }

    return get_property(dpch, dpid, result);
}

HRESULT get_property_v(IDispatch *dpch, LPOLESTR *name, VARIANT *result)
{
    IDispatch *d, *dd;
    HRESULT hres;

    d = dpch;
    IDispatch_AddRef(d);

    while(name[0] != NULL) {
        if(name[1] != NULL) {
            hres = get_property_dp(d, name[0], &dd);
            IDispatch_Release(d);
            if(FAILED(hres)) {
                return hres;
            }

            d = dd;
        } else {
            hres = get_property_s(d, name[0], result);
            IDispatch_Release(d);
            if(FAILED(hres)) {
                return hres;
            }
        }

        name += 1;
    }

    return S_OK;
}

HRESULT get_property_dp(IDispatch *dpch, LPOLESTR name, IDispatch **result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property_s(dpch, name, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_DISPATCH) {
        return E_FAIL;
    }

    *result = var.pdispVal;

    return S_OK;
}

HRESULT get_property_dpd(IDispatch *dpch, DISPID dpid, IDispatch **result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property(dpch, dpid, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_DISPATCH) {
        return E_FAIL;
    }

    *result = var.pdispVal;

    return S_OK;
}

HRESULT get_property_dpv(IDispatch *dpch, LPOLESTR *name, IDispatch **result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property_v(dpch, name, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_DISPATCH) {
        return E_FAIL;
    }

    *result = var.pdispVal;

    return S_OK;
}

HRESULT get_property_str(IDispatch *dpch, LPOLESTR name, BSTR *result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property_s(dpch, name, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_BSTR) {
        return E_FAIL;
    }

    *result = var.bstrVal;

    return S_OK;
}

HRESULT get_property_strd(IDispatch *dpch, DISPID dpid, BSTR *result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property(dpch, dpid, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_BSTR) {
        return E_FAIL;
    }

    *result = var.bstrVal;

    return S_OK;
}

HRESULT get_property_strv(IDispatch *dpch, LPOLESTR *name, BSTR *result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property_v(dpch, name, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_BSTR) {
        return E_FAIL;
    }

    *result = var.bstrVal;

    return S_OK;
}

HRESULT get_property_long(IDispatch *dpch, LPOLESTR name, long *result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property_s(dpch, name, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_I4) {
        return E_FAIL;
    }

    *result = var.lVal;

    return S_OK;
}

HRESULT get_property_longd(IDispatch *dpch, DISPID dpid, long *result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property(dpch, dpid, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_I4) {
        return E_FAIL;
    }

    *result = var.lVal;

    return S_OK;
}

HRESULT get_property_longv(IDispatch *dpch, LPOLESTR *name, long *result)
{
    VARIANT var;
    HRESULT hres;

    hres = get_property_v(dpch, name, &var);
    if(FAILED(hres)) {
        return hres;
    }

    if(var.vt != VT_I4) {
        return E_FAIL;
    }

    *result = var.lVal;

    return S_OK;
}


HRESULT put_property_s(IDispatch *dpch, LPOLESTR name, VARIANTARG *val)
{
    DISPID dpid;
    HRESULT hres;

    hres = get_dpid(dpch, name, &dpid);
    if(FAILED(hres)) {
        return hres;
    }

    return put_property(dpch, dpid, val);
}

HRESULT put_property_v(IDispatch *dpch, LPOLESTR *name, VARIANTARG *val)
{
    IDispatch *d, *dd;
    HRESULT hres;

    d = dpch;
    IDispatch_AddRef(d);

    while(name[0] != NULL) {
        if(name[1] != NULL) {
            hres = get_property_dp(d, name[0], &dd);
            IDispatch_Release(d);
            if(FAILED(hres)) {
                return hres;
            }

            d = dd;
        } else {
            hres = put_property_s(d, name[0], val);
            IDispatch_Release(d);
            if(FAILED(hres)) {
                return hres;
            }
        }

        name += 1;
    }

    return S_OK;
}

HRESULT put_property_str(IDispatch *dpch, LPOLESTR name, BSTR val)
{
    VARIANTARG arg;

    arg.vt = VT_BSTR;
    arg.bstrVal = val;

    return put_property_s(dpch, name, &arg);
}

HRESULT put_property_strv(IDispatch *dpch, LPOLESTR *name, BSTR val)
{
    VARIANTARG arg;

    arg.vt = VT_BSTR;
    arg.bstrVal = val;

    return put_property_v(dpch, name, &arg);
}

HRESULT put_property_long(IDispatch *dpch, LPOLESTR name, long val)
{
    VARIANTARG arg;

    arg.vt = VT_I4;
    arg.lVal = val;

    return put_property_s(dpch, name, &arg);
}

HRESULT put_property_longv(IDispatch *dpch, LPOLESTR *name, long val)
{
    VARIANTARG arg;

    arg.vt = VT_I4;
    arg.lVal = val;

    return put_property_v(dpch, name, &arg);
}

HRESULT put_property_longd(IDispatch *dpch, DISPID dpid, long val)
{
    VARIANTARG arg;

    arg.vt = VT_I4;
    arg.lVal = val;

    return put_property(dpch, dpid, &arg);
}
