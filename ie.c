/*
 * ie.c  -- ie compornent operation
 *
 * $Id: ie.c,v 1.1 2005/01/04 09:36:13 hos Exp $
 *
 */

#include "main.h"
#include "automation.h"
#include <oleacc.h>
#include <stdio.h>


static
HRESULT get_ie_elem_at(IDispatch *doc, int x, int y, IDispatch **ret_doc)
{
    IDispatch *inner_doc;
    VARIANTARG args[2];
    VARIANT var;
    HRESULT hres;

    inner_doc = doc;
    IDispatch_AddRef(inner_doc);

    while(1) {
        IDispatch *win, *elem;
        BSTR str;

        hres = get_property_dp(inner_doc, L"parentWindow", &win);
        if(FAILED(hres)) {
            printf("parent fail: %x\n", hres); fflush(stdout);
            IDispatch_Release(inner_doc);
            return hres;
        }

        {
            long sx = 0, sy = 0;

            get_property_long(win, L"screenLeft", &sx);
            get_property_long(win, L"screenTop", &sy);
            IDispatch_Release(win);
            printf("sx, sy: %d, %d\n", sx, sy); fflush(stdout);

            args[0].vt = VT_I4;
            args[0].lVal = y - sy;
            args[1].vt = VT_I4;
            args[1].lVal = x - sx;
        }

        hres = call_method_s(inner_doc, L"elementFromPoint", args, 2, &var);
        if(FAILED(hres)) {
            IDispatch_Release(inner_doc);
            return hres;
        }

        if(var.vt != VT_DISPATCH) {
            IDispatch_Release(inner_doc);
            return E_FAIL;
        }

        elem = var.pdispVal;

        hres = get_property_str(elem, L"tagName", &str);
        if(FAILED(hres)) {
            IDispatch_Release(inner_doc);
            IDispatch_Release(elem);
            return hres;
        }

        if(str == NULL || wcsstr(str, L"FRAME") == NULL) {
            SysFreeString(str);
            IDispatch_Release(elem);
            break;
        }

        SysFreeString(str);
        IDispatch_Release(inner_doc);

        {
            LPOLESTR props[] = {L"contentWindow", L"document", NULL};
            hres = get_property_dpv(elem, props, &inner_doc);
        }
        IDispatch_Release(elem);
        if(FAILED(hres)) {
            return hres;
        }
    }

    *ret_doc = inner_doc;

    return S_OK;
}


HRESULT get_ie_target(HWND hwnd, int x, int y, IDispatch **elem)
{
    IDispatch *top_doc;
    UINT getobj_msg;
    LRESULT lres, res;
    HRESULT hres;

    getobj_msg = RegisterWindowMessage(_T("WM_HTML_GETOBJECT"));
    if(getobj_msg == 0) {
        return E_FAIL;
    }

    lres = SendMessageTimeout(hwnd, getobj_msg, 0, 0,
                              SMTO_ABORTIFHUNG, 1000,
                              (PDWORD_PTR)&res);
    if(hres == 0) {
        return E_FAIL;
    }

    hres = ObjectFromLresult(res, &IID_IDispatch, 0,
                             (void **)(void *)&top_doc);
    if(FAILED(hres)) {
        return hres;
    }

    hres = get_ie_elem_at(top_doc, x, y, elem);
    if(FAILED(hres)) {
        IDispatch_Release(top_doc);
        return hres;
    }

    return S_OK;
}
