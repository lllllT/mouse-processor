/*
 * ie.c  -- ie compornent operation
 *
 * $Id: ie.c,v 1.3 2005/01/05 06:55:25 hos Exp $
 *
 */

#include "main.h"
#include "automation.h"
#include <oleacc.h>


static
HRESULT get_ie_elem_at(IDispatch *doc, int x, int y, IDispatch **ret_elem)
{
    IDispatch *inner_doc;
    VARIANTARG args[2];
    VARIANT var;
    HRESULT hres;

    inner_doc = doc;
    IDispatch_AddRef(inner_doc);

    while(1) {
        {
            IDispatch *win;

            hres = get_property_dp(inner_doc, L"parentWindow", &win);
            if(FAILED(hres)) {
                IDispatch_Release(inner_doc);
                return hres;
            }

            {
                long sx = 0, sy = 0;

                get_property_long(win, L"screenLeft", &sx);
                get_property_long(win, L"screenTop", &sy);
                IDispatch_Release(win);

                args[0].vt = VT_I4;
                args[0].lVal = y - sy;
                args[1].vt = VT_I4;
                args[1].lVal = x - sx;
            }
        }

        hres = call_method_s(inner_doc, L"elementFromPoint", args, 2, &var);
        IDispatch_Release(inner_doc);
        if(FAILED(hres)) {
            return hres;
        }

        if(var.vt != VT_DISPATCH) {
            return E_FAIL;
        }

        {
            IDispatch *elem = var.pdispVal;

            {
                BSTR str;

                hres = get_property_str(elem, L"tagName", &str);
                if(FAILED(hres)) {
                    IDispatch_Release(elem);
                    return hres;
                }

                if(str == NULL || wcsstr(str, L"FRAME") == NULL) {
                    SysFreeString(str);

                    *ret_elem = elem;
                    break;
                }

                SysFreeString(str);
            }

            {
                static LPOLESTR props[] =
                    {L"contentWindow", L"document", NULL};
                hres = get_property_dpv(elem, props, &inner_doc);
            }
            if(FAILED(hres)) {
                *ret_elem = elem;
                break;
            }

            IDispatch_Release(elem);
        }
    }

    return S_OK;
}


HRESULT get_ie_target(HWND hwnd, int x, int y, IDispatch **ret_elem)
{
    IDispatch *top_doc, *elem;
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
    if(lres == 0) {
        return E_FAIL;
    }

    hres = ObjectFromLresult(res, &IID_IDispatch, 0,
                             (void **)(void *)&top_doc);
    if(FAILED(hres)) {
        return hres;
    }

    hres = get_ie_elem_at(top_doc, x, y, &elem);
    if(FAILED(hres)) {
        IDispatch_Release(top_doc);
        return hres;
    }

    while(1) {
        BSTR overflow = NULL, tagname = NULL;
        static LPOLESTR props[] = {L"style", L"overflow", NULL};

        if(SUCCEEDED(get_property_strv(elem, props, &overflow)) &&
           SUCCEEDED(get_property_str(elem, L"tagName", &tagname))) {
            if((tagname != NULL &&
                ((wcscmp(tagname, L"BODY") == 0 ||
                  wcscmp(tagname, L"TEXTAREA") == 0) &&
                 overflow == NULL)) ||
               (overflow != NULL &&
                (wcscmp(overflow, L"auto") == 0 ||
                 wcscmp(overflow, L"scroll") == 0))) {
                long cw, ch, sw, sh;

                SysFreeString(overflow);
                SysFreeString(tagname);

                if(SUCCEEDED(get_property_long(elem, L"clientWidth", &cw)) &&
                   SUCCEEDED(get_property_long(elem, L"clientHeight", &ch)) &&
                   SUCCEEDED(get_property_long(elem, L"scrollWidth", &sw)) &&
                   SUCCEEDED(get_property_long(elem, L"scrollHeight", &sh))) {
                    if(cw != 0 && ch != 0 && (cw != sw || ch != sh)) {
                        break;
                    }
                }
            }
        }
        SysFreeString(overflow);
        SysFreeString(tagname);

        {
            IDispatch *parent;

            hres = get_property_dp(elem, L"parentElement", &parent);
            IDispatch_Release(elem);
            if(FAILED(hres)) {
                return hres;
            }

            if(parent == NULL) {
                return E_FAIL;
            }

            elem = parent;
        }
    }

    *ret_elem = elem;

    return S_OK;
}

HRESULT get_ie_elem_size(IDispatch *elem, SIZE *sz)
{
    long cw, ch;

    if(FAILED(get_property_long(elem, L"clientWidth", &cw)) ||
       FAILED(get_property_long(elem, L"clientHeight", &ch))) {
        return E_FAIL;
    }

    sz->cx = cw;
    sz->cy = ch;

    return S_OK;
}

static DISPID elem_scroll_left, elem_client_width, elem_scroll_width;
static DISPID elem_scroll_top, elem_client_height, elem_scroll_height;

HRESULT init_ie_dpids(IDispatch *elem)
{
    HRESULT hres;
    int i;

    struct {
        LPOLESTR name;
        DISPID *dpid;
    } names[] = {
        {L"scrollLeft", &elem_scroll_left},
        {L"clientWidth", &elem_client_width},
        {L"scrollWidth", &elem_scroll_width},
        {L"scrollTop", &elem_scroll_top},
        {L"clientHeight", &elem_client_height},
        {L"scrollHeight", &elem_scroll_height},

        {NULL, NULL}
    };

    for(i = 0; names[i].name != NULL; i++) {
        hres = get_dpid(elem, names[i].name, names[i].dpid);
        if(FAILED(hres)) {
            return hres;
        }
    }

    return S_OK;
}


static
int scroll_ie(IDispatch *elem, int delta, int length,
              DISPID pos_id, DISPID page_id, DISPID all_id)
{
    long pos, page, all, range;
    double page_ratio, spd;

    if(FAILED(get_property_longd(elem, pos_id, &pos)) ||
       FAILED(get_property_longd(elem, page_id, &page)) ||
       FAILED(get_property_longd(elem, all_id, &all))) {
        return 0;
    }

    if(all == 0 || page >= all) {
        return 0;
    }

    range = all - page + 1;
    page_ratio = (double)page / all;
    spd = range / (length * (1 - page_ratio));

    delta *= spd;
    if(delta == 0) {
        return 0;
    }

    pos += delta;
    if(pos < 0) {
        pos = 0;
    } else if(pos > all - page - 1) {
        pos = all - page - 1;
    }

    if(FAILED(put_property_longd(elem, pos_id, pos))) {
        return 0;
    }

    return 1;
}

int scroll_ie_h(IDispatch *elem, int delta, int length)
{
    return scroll_ie(elem, delta, length,
                     elem_scroll_left, elem_client_width, elem_scroll_width);
}

int scroll_ie_v(IDispatch *elem, int delta, int length)
{
    return scroll_ie(elem, delta, length,
                     elem_scroll_top, elem_client_height, elem_scroll_height);
}
