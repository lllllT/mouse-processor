/*
 * regexp.c  -- regular expression
 *
 * $Id: regexp.c,v 1.1 2005/01/13 09:39:55 hos Exp $
 *
 */

#include "main.h"
#include "automation.h"


static IDispatch *regexp = NULL;


int is_regexp_match(BSTR re_str, BSTR test_str)
{
    HRESULT hres;
    VARIANTARG arg;
    VARIANT var;

    if(regexp == NULL) {
        hres = create_instance_from_progid(L"VBScript.RegExp", &regexp);
        if(FAILED(hres)) {
            return 0;
        }
    }

    hres = put_property_str(regexp, L"Pattern", re_str);
    if(FAILED(hres)) {
        return 0;
    }

    arg.vt = VT_BSTR;
    arg.bstrVal = test_str;
    hres = call_method_s(regexp, L"Test", &arg, 1, &var);
    if(FAILED(hres)) {
        return 0;
    }

    if(var.vt != VT_BOOL) {
        return 0;
    }

    return var.boolVal;
}
