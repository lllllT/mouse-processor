/*
 * automation.h  -- COM automation helper
 *
 * $Id: automation.h,v 1.1 2005/01/05 07:46:31 hos Exp $
 *
 */

#include <objbase.h>

HRESULT get_active_object(LPCOLESTR progid, IDispatch **dpch);

HRESULT get_dpid(IDispatch *dpch, LPOLESTR name, DISPID *dpid);

HRESULT invoke(IDispatch *dpch, DISPID dpid, WORD flags,
               DISPPARAMS *params, VARIANT *result);

HRESULT call_method(IDispatch *dpch, DISPID dpid,
                    VARIANTARG *args, unsigned int nargs, VARIANT *result);
HRESULT get_property(IDispatch *dpch, DISPID dpid, VARIANT *result);
HRESULT put_property(IDispatch *dpch, DISPID dpid, VARIANTARG *val);

HRESULT call_method_s(IDispatch *dpch, LPOLESTR name,
                      VARIANTARG *args, unsigned int nargs, VARIANT *result);

HRESULT get_property_s(IDispatch *dpch, LPOLESTR name, VARIANT *result);
HRESULT get_property_v(IDispatch *dpch, LPOLESTR *name, VARIANT *result);
HRESULT get_property_dp(IDispatch *dpch, LPOLESTR name, IDispatch **result);
HRESULT get_property_dpd(IDispatch *dpch, DISPID dpid, IDispatch **result);
HRESULT get_property_dpv(IDispatch *dpch, LPOLESTR *name, IDispatch **result);
HRESULT get_property_str(IDispatch *dpch, LPOLESTR name, BSTR *result);
HRESULT get_property_strd(IDispatch *dpch, DISPID dpid, BSTR *result);
HRESULT get_property_strv(IDispatch *dpch, LPOLESTR *name, BSTR *result);
HRESULT get_property_long(IDispatch *dpch, LPOLESTR name, long *result);
HRESULT get_property_longd(IDispatch *dpch, DISPID dpid, long *result);
HRESULT get_property_longv(IDispatch *dpch, LPOLESTR *name, long *result);

HRESULT put_property_s(IDispatch *dpch, LPOLESTR name, VARIANTARG *val);
HRESULT put_property_v(IDispatch *dpch, LPOLESTR *name, VARIANTARG *val);
HRESULT put_property_str(IDispatch *dpch, LPOLESTR name, BSTR val);
HRESULT put_property_strv(IDispatch *dpch, LPOLESTR *name, BSTR val);
HRESULT put_property_long(IDispatch *dpch, LPOLESTR name, long val);
HRESULT put_property_longd(IDispatch *dpch, DISPID dpid, long val);
HRESULT put_property_longv(IDispatch *dpch, LPOLESTR *name, long val);
