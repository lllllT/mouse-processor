/*
 * regexp.c  -- regular expression
 *
 */

#include "main.h"
#include "automation.h"


static IDispatch *regexp = NULL;


HRESULT init_regexp(void)
{
    HRESULT hres;

    hres = create_instance_from_progid(L"VBScript.RegExp", &regexp);
    if(FAILED(hres)) {
        return hres;
    }

    hres = put_property_long(regexp, L"IgnoreCase", 1);
    if(FAILED(hres)) {
        return hres;
    }

    return S_OK;
}

int is_regexp_match(BSTR re_str, BSTR test_str)
{
    HRESULT hres;
    VARIANTARG arg;
    VARIANT var;

    hres = put_property_str(regexp, L"Pattern", re_str);
    if(FAILED(hres)) {
        error_message_hr("Failed to set RegExp.Pattern", hres);
        return 0;
    }

    arg.vt = VT_BSTR;
    arg.bstrVal = test_str;
    hres = call_method_s(regexp, L"Test", &arg, 1, &var);
    if(FAILED(hres)) {
        error_message_hr("Failed to call RegExp.Test", hres);
        return 0;
    }

    if(var.vt != VT_BOOL) {
        return 0;
    }

    return var.boolVal;
}
