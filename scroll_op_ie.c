/*
 * scroll_op_ie.c  -- scroll operator for IE component
 *
 */

#define COBJMACROS 1

#include "operator.h"
#include "scroll_op_utils.h"
#include "util.h"
#include "automation.h"
#include <oleacc.h>
#include <math.h>


static const support_procs_t *spr = NULL;


struct ie_scroll_dpids {
    DISPID scroll_pos;
    DISPID page_length;
    DISPID scroll_total;
};


static
HRESULT get_ie_elem_at(IDispatch *doc, int x, int y, IDispatch **ret_elem)
{
    IDispatch *inner_doc;
    VARIANTARG args[2];
    VARIANT var;
    HRESULT hres;

    inner_doc = doc;
    IDispatch_AddRef(inner_doc);

    {
        IDispatch *win;

        hres = get_property_dp(inner_doc, L"parentWindow", &win);
        if(FAILED(hres)) {
            IDispatch_Release(inner_doc);

            spr->log_hresult(
                LOG_LEVEL_NOTIFY,
                L"ie-scroll: Failed to get document.parentWindow",
                hres, 1);
            return hres;
        }

        {
            long sx = 0, sy = 0;

            get_property_long(win, L"screenLeft", &sx);
            get_property_long(win, L"screenTop", &sy);
            IDispatch_Release(win);

            x -= sx;
            y -= sy;
        }
    }

    while(1) {
        IDispatch *elem;

        args[0].vt = VT_I4;
        args[0].lVal = y;
        args[1].vt = VT_I4;
        args[1].lVal = x;

        hres = call_method_s(inner_doc, L"elementFromPoint", args, 2, &var);
        IDispatch_Release(inner_doc);
        if(FAILED(hres)) {
            spr->log_hresult(
                LOG_LEVEL_NOTIFY,
                L"ie-scroll: Failed to get document.elementFromPoint",
                hres, 1);
            return hres;
        }

        if(var.vt != VT_DISPATCH) {
            spr->log_printf(
                LOG_LEVEL_NOTIFY,
                L"ie-scroll: Failed to get document.elementFromPoint: "
                L"return value is not IDispatch\n");
            return E_FAIL;
        }

        elem = var.pdispVal;
        if(elem == NULL) {
            spr->log_printf(
                LOG_LEVEL_NOTIFY,
                L"ie-scroll: Failed to get document.elementFromPoint: "
                L"return value is NULL\n");
            return E_FAIL;
        }

        {
            BSTR str;

            hres = get_property_str(elem, L"tagName", &str);
            if(FAILED(hres)) {
                IDispatch_Release(elem);

                spr->log_hresult(
                    LOG_LEVEL_NOTIFY,
                    L"ie-scroll: Failed to get element.tagName",
                    hres, 1);
                return hres;
            }
            spr->log_printf(LOG_LEVEL_DEBUG,
                            L"ie-scroll: tagName of target element: %ls\n",
                            str);

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

            spr->log_hresult(
                LOG_LEVEL_NOTIFY,
                L"ie-scroll: Failed to get "
                L"frameElement.contentWindow.document",
                hres, 1);
            break;
        }

        while(1) {
            IDispatch *parent = NULL;
            long ox = 0, oy = 0, cx = 0, cy = 0, sx = 0, sy = 0;
            BSTR tp = NULL;

            get_property_long(elem, L"clientLeft", &cx);
            get_property_long(elem, L"clientTop", &cy);
            get_property_long(elem, L"offsetLeft", &ox);
            get_property_long(elem, L"offsetTop", &oy);
            get_property_long(elem, L"scrollLeft", &sx);
            get_property_long(elem, L"scrollTop", &sy);

            x -= cx + ox - sx;
            y -= cy + oy - sy;

            hres = get_property_dp(elem, L"offsetParent", &parent);
            if(FAILED(hres) || parent == NULL) {
                break;
            }

            get_property_str(parent, L"tagName", &tp);
            if(tp != NULL && wcscmp(tp, L"FRAMESET") == 0) {
                SysFreeString(tp);
                IDispatch_Release(parent);
                break;
            }
            SysFreeString(tp);

            IDispatch_Release(elem);
            elem = parent;
        }

        while(1) {
            IDispatch *parent = NULL;
            long sx = 0, sy = 0;

            hres = get_property_dp(elem, L"parentElement", &parent);
            if(FAILED(hres) || parent == NULL) {
                break;
            }

            get_property_long(parent, L"scrollLeft", &sx);
            get_property_long(parent, L"scrollTop", &sy);

            x += sx;
            y += sy;

            IDispatch_Release(elem);
            elem = parent;
        }

        IDispatch_Release(elem);
    }

    return S_OK;
}

static
HRESULT get_scrollable_parent(IDispatch *elem, IDispatch **ret_elem)
{
    HRESULT hres;

    IDispatch_AddRef(elem);

    while(1) {
        BSTR overflow = NULL, curoverflow = NULL, tagname = NULL;
        BSTR compatmode = NULL;
        static LPOLESTR props[] = {L"style", L"overflow", NULL};
        static LPOLESTR curprops[] = {L"currentStyle", L"overflow", NULL};
        static LPOLESTR compatmode_props[] = {
            L"ownerDocument", L"compatMode", NULL };

        if(SUCCEEDED(get_property_strv(elem, props, &overflow)) &&
           SUCCEEDED(get_property_strv(elem, curprops, &curoverflow)) &&
           SUCCEEDED(get_property_str(elem, L"tagName", &tagname))) {
            if(overflow == NULL) {
                overflow = curoverflow;
                curoverflow = NULL;
            }

            if((tagname != NULL &&
                (wcscmp(tagname, L"HTML") == 0 ||
                 wcscmp(tagname, L"BODY") == 0 ||
                 wcscmp(tagname, L"TEXTAREA") == 0) &&
                (overflow == NULL ||
                 wcscmp(overflow, L"visible") == 0)) ||
               (overflow != NULL &&
                (wcscmp(overflow, L"auto") == 0 ||
                 wcscmp(overflow, L"scroll") == 0))) {
                long cw, ch, sw, sh;

                if(tagname != NULL &&
                   wcscmp(tagname, L"BODY") == 0 &&
                   SUCCEEDED(get_property_strv(elem, compatmode_props,
                                               &compatmode)) &&
                   compatmode != NULL &&
                   wcscmp(compatmode, L"CSS1Compat") == 0) {
                    /* workaround for IE7:
                       ignore BODY element if DOCTYPE is exists */
                }
                else if(
                    SUCCEEDED(get_property_long(elem, L"clientWidth", &cw)) &&
                    SUCCEEDED(get_property_long(elem, L"clientHeight", &ch)) &&
                    SUCCEEDED(get_property_long(elem, L"scrollWidth", &sw)) &&
                    SUCCEEDED(get_property_long(elem, L"scrollHeight", &sh))) {
                    if(cw != 0 && ch != 0 && (cw < sw || ch < sh)) {
                        spr->log_printf(
                            LOG_LEVEL_DEBUG,
                            L"ie-scroll: tagName of scroll target element: "
                            L"%ls\n",
                            tagname);

                        SysFreeString(overflow);
                        SysFreeString(curoverflow);
                        SysFreeString(tagname);
                        SysFreeString(compatmode);
                        break;
                    }
                }
            }
        }
        SysFreeString(overflow);
        SysFreeString(curoverflow);
        SysFreeString(tagname);
        SysFreeString(compatmode);

        {
            IDispatch *parent;

            hres = get_property_dp(elem, L"parentElement", &parent);
            IDispatch_Release(elem);
            if(FAILED(hres)) {
                spr->log_hresult(
                    LOG_LEVEL_NOTIFY,
                    L"ie-scroll: Failed to get element.parentElement",
                    hres, 1);
                return hres;
            }

            if(parent == NULL) {
                spr->log_printf(
                    LOG_LEVEL_DEBUG,
                    L"ie-scroll: scroll target element is not found\n");
                return E_FAIL;
            }

            elem = parent;
        }
    }

    *ret_elem = elem;

    return S_OK;
}

static
IDispatch *get_ie_target(HWND hwnd, int x, int y)
{
    IDispatch *top_doc, *elem, *target;
    UINT getobj_msg;
    LRESULT lres, res;
    HRESULT hres;

    getobj_msg = RegisterWindowMessageW(L"WM_HTML_GETOBJECT");
    if(getobj_msg == 0) {
        spr->log_lasterror(
            LOG_LEVEL_NOTIFY,
            L"ie-scroll: Failed to call "
            L"RegisterWindowMessage(\"WM_HTML_GETOBJECT\")", 1);
        return NULL;
    }

    lres = SendMessageTimeout(hwnd, getobj_msg, 0, 0,
                              SMTO_ABORTIFHUNG, 1000,
                              (PDWORD_PTR)&res);
    if(lres == 0) {
        spr->log_lasterror(
            LOG_LEVEL_NOTIFY,
            L"ie-scroll: Failed to call SendMessageTimeout()", 1);
        return NULL;
    }

    hres = ObjectFromLresult(res, &IID_IDispatch, 0,
                             (void **)(void *)&top_doc);
    if(FAILED(hres)) {
        spr->log_hresult(
            LOG_LEVEL_NOTIFY,
            L"ie-scroll: Failed to call ObjectFromLresult()",
            hres, 1);
        return NULL;
    }

    hres = get_ie_elem_at(top_doc, x, y, &elem);
    IDispatch_Release(top_doc);
    if(FAILED(hres)) {
        return NULL;
    }

    hres = get_scrollable_parent(elem, &target);
    IDispatch_Release(elem);
    if(FAILED(hres)) {
        return NULL;
    }

    return target;
}

static
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

static
HRESULT get_ie_dpids(IDispatch *elem,
                     struct ie_scroll_dpids *h_dpids,
                     struct ie_scroll_dpids *v_dpids)
{
    HRESULT hres;
    int i;

    struct {
        LPOLESTR name;
        DISPID *dpid;
    } names[] = {
        {L"scrollLeft", &h_dpids->scroll_pos},
        {L"clientWidth", &h_dpids->page_length},
        {L"scrollWidth", &h_dpids->scroll_total},
        {L"scrollTop", &v_dpids->scroll_pos},
        {L"clientHeight", &v_dpids->page_length},
        {L"scrollHeight", &v_dpids->scroll_total},

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



typedef int (* ie_scroll_proc_t)(IDispatch *,
                                 const struct ie_scroll_dpids *,
                                 double *, int);

static
int ie_scroll_r_scroll(IDispatch *elem,
                       const struct ie_scroll_dpids *dpids,
                       double *delta, int length,
                       int mode)
{
    long pos, page, total;
    long dd, min, max;
    double rest;

    if(FAILED(get_property_longd(elem, dpids->scroll_pos, &pos)) ||
       FAILED(get_property_longd(elem, dpids->page_length, &page)) ||
       FAILED(get_property_longd(elem, dpids->scroll_total, &total))) {
        return 0;
    }

    if(total == 0 || page >= total) {
        return 0;
    }

    min = 0;
    max = total - page;

    if(mode == 0) {
        dd = get_drag_scroll_delta(length, min, max, (double)page / total,
                                   *delta, &rest);
    } else if(mode == 1) {
        dd = trunc((*delta) / 100.0 * (max - min));
    } else {
        dd = trunc(*delta);
    }

    if(dd == 0) {
        return 1;
    }

    pos += dd;
    if(pos < min) {
        pos = min;
    } else if(pos > max) {
        pos = max;
    }

    put_property_longd(elem, dpids->scroll_pos, pos);

    if(mode == 0) {
        *delta = rest;
    } else if(mode == 1) {
        *delta -= dd * 100.0 / (max - min);
    } else {
        *delta -= dd;
    }

    return 1;
}

static
int ie_scroll_drag_scroll(IDispatch *elem,
                          const struct ie_scroll_dpids *dpids,
                          double *delta, int length)
{
    return ie_scroll_r_scroll(elem, dpids, delta, length, 0);
}

static
int ie_scroll_percentage_scroll(IDispatch *elem,
                                const struct ie_scroll_dpids *dpids,
                                double *delta, int length)
{
    return ie_scroll_r_scroll(elem, dpids, delta, length, 1);
}

static
int ie_scroll_pixel_scroll(IDispatch *elem,
                           const struct ie_scroll_dpids *dpids,
                           double *delta, int length)
{
    return ie_scroll_r_scroll(elem, dpids, delta, length, 2);
}


enum {
    IE_SCROLL_MODE_DRAG = 0,
    IE_SCROLL_MODE_PERCENTAGE,
    IE_SCROLL_MODE_PIXEL
};

static struct scroll_op_mode_pair ie_scroll_mode_map[] = {
    {L"drag", IE_SCROLL_MODE_DRAG},
    {L"percentage", IE_SCROLL_MODE_PERCENTAGE},
    {L"pixel", IE_SCROLL_MODE_PIXEL},

    {NULL, 0}
};

static struct uint_ptr_pair ie_scroll_proc_map[] = {
    {IE_SCROLL_MODE_DRAG, ie_scroll_drag_scroll},
    {IE_SCROLL_MODE_PERCENTAGE, ie_scroll_percentage_scroll},
    {IE_SCROLL_MODE_PIXEL, ie_scroll_pixel_scroll},

    {0, NULL}
};


struct ie_scroll_context {
    int mode;
    ie_scroll_proc_t scroll_proc;

    IDispatch *target;
    SIZE target_size;
    struct ie_scroll_dpids h_dpids;
    struct ie_scroll_dpids v_dpids;

    double x_ratio, y_ratio;
    double dx, dy;
};

static
int MP_OP_API ie_scroll_get_ctx_size(const op_arg_t *arg)
{
    return sizeof(struct ie_scroll_context);
}

static
int MP_OP_API ie_scroll_init_ctx(void *ctxp, int size, const op_arg_t *arg)
{
    struct ie_scroll_context *ctx;
    s_exp_data_t *mode_conf;
    wchar_t *mode_name;

    if(size != sizeof(struct ie_scroll_context)) {
        return 0;
    }

    ctx = (struct ie_scroll_context *)ctxp;

    /* target element */
    ctx->target = get_ie_target(arg->hwnd, arg->pos.x, arg->pos.y);
    if(ctx->target == NULL) {
        return 0;
    }

    /* scroll mode name */
    mode_name = get_scroll_op_mode_name(arg->arg, arg->conf, L"drag");

    /* scroll mode */
    ctx->mode = get_scroll_op_mode(ie_scroll_mode_map, mode_name);
    if(ctx->mode < 0) {
        spr->log_printf(LOG_LEVEL_DEBUG,
                        L"ie-scroll: unknown mode: %ls\n", mode_name);
        return 0;
    }

    ctx->scroll_proc = (ie_scroll_proc_t)
                       assq_pair(ie_scroll_proc_map, ctx->mode, NULL);

    /* x and y ratio */
    mode_conf = s_exp_massq(arg->conf, S_EXP_TYPE_CONS, mode_name, NULL);
    if(mode_conf == NULL) {
        mode_conf = S_EXP_NIL;
    }

    get_scroll_op_xy_ratio(s_exp_nth_cdr(arg->arg, 1), mode_conf,
                           &ctx->x_ratio, &ctx->y_ratio, 1.0, 1.0);

    /* window size */
    if(FAILED(get_ie_elem_size(ctx->target, &ctx->target_size))) {
        return 0;
    }

    /* get DISPID */
    if(FAILED(get_ie_dpids(ctx->target, &ctx->h_dpids, &ctx->v_dpids))) {
        return 0;
    }

    /* initial delta */
    ctx->dx = 0;
    ctx->dy = 0;

    return 1;
}

static
int MP_OP_API ie_scroll_scroll(void *ctxp, double dx, double dy)
{
    struct ie_scroll_context *ctx;

    ctx = (struct ie_scroll_context *)ctxp;

    ctx->dx += dx * ctx->x_ratio;
    ctx->dy += dy * ctx->y_ratio;

    ctx->scroll_proc(ctx->target, &ctx->h_dpids,
                     &ctx->dx, ctx->target_size.cx);
    ctx->scroll_proc(ctx->target, &ctx->v_dpids,
                     &ctx->dy, ctx->target_size.cy);

    return 1;
}

static
int MP_OP_API ie_scroll_end_scroll(void *ctxp)
{
    struct ie_scroll_context *ctx;

    ctx = (struct ie_scroll_context *)ctxp;

    IDispatch_Release(ctx->target);

    return 1;
}

int MP_OP_API ie_scroll_get_operator(scroll_op_procs_t *op, int size,
                                     const support_procs_t *sprocs)
{
    spr = sprocs;

    if(size < sizeof(scroll_op_procs_t)) {
        return 0;
    }

    op->hdr.api_ver = MP_OP_API_VERSION;
    op->hdr.type = MP_OP_TYPE_SCROLL;

    op->get_context_size = ie_scroll_get_ctx_size;
    op->init_context = ie_scroll_init_ctx;
    op->scroll = ie_scroll_scroll;
    op->end_scroll = ie_scroll_end_scroll;

    return 1;
}
