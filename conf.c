/*
 * conf.h  -- configuration
 *
 * $Id: conf.c,v 1.9 2005/01/14 14:54:36 hos Exp $
 *
 */

#include "main.h"
#include "util.h"
#include <stdio.h>
#include <stdarg.h>


#define HOME_ENV L"HOME"
#define HOME_RC_NAME L"\\.mprc"
#define MODULE_RC_NAME L"\\default.mprc"


static
LPWSTR get_home_path(void)
{
    DWORD size;
    LPWSTR path;

    size = GetEnvironmentVariableW(HOME_ENV, NULL, 0);
    if(size == 0) {
        return NULL;
    }
    size += wcslen(HOME_RC_NAME);

    path = (LPWSTR)malloc(sizeof(WCHAR) * size);
    if(path == NULL) {
        return NULL;
    }
    memset(path, 0, sizeof(WCHAR) * size);

    GetEnvironmentVariable(HOME_ENV, path, size);

    wcscat(path, HOME_RC_NAME);

    return path;
}

static
LPWSTR get_module_path(void)
{
    DWORD size;
    LPWSTR path;
    WCHAR buf[512];

    size = GetModuleFileName(NULL, buf, 512);
    if(size == 0) {
        return NULL;
    }
    size += wcslen(MODULE_RC_NAME);

    path = (LPWSTR)malloc(sizeof(WCHAR) * size);
    if(path == NULL) {
        return NULL;
    }

    wcscpy(path, buf);
    {
        LPWSTR p;

        p = (LPWSTR)wcsrchr(path, L'\\');
        if(p != NULL) {
            *p = 0;
        }
    }

    wcscat(path, MODULE_RC_NAME);

    return path;
}


static
s_exp_data_t *load_file_conf(LPCWSTR path)
{
    char *u8s_path = NULL;
    FILE *fp = NULL;
    s_exp_read_context_t *rc = NULL;
    s_exp_data_t *data = NULL;

    u8s_path = u8s_dup_from_wcs(path);
    if(u8s_path == NULL) {
        goto end;
    }

    fp = _wfopen(path, L"r");
    if(fp == NULL) {
        goto end;
    }

    rc = open_s_exp_read_context_f(fp, u8s_path);
    if(rc == NULL) {
        goto end;
    }

    data = read_all_s_exp(rc);

  end:
    if(u8s_path != NULL) free(u8s_path);
    if(fp != NULL) fclose(fp);
    if(rc != NULL) close_s_exp_read_context(rc);

    return data;
}

s_exp_data_t *load_conf(LPCWSTR conf_file)
{
    LPWSTR path;
    s_exp_data_t *data;

    if(conf_file != NULL) {
        return load_file_conf(conf_file);
    }

    if((path = get_home_path()) != NULL) {
        data = load_file_conf(path);
        free(path);

        if(data != NULL) {
            return data;
        }
    }

    if((path = get_module_path()) != NULL) {
        data = load_file_conf(path);
        free(path);

        if(data != NULL) {
            return data;
        }
    }

    return NULL;
}


static
s_exp_data_t *get_conf_v(int type, va_list ap)
{
    return s_exp_massq_v(ctx.app_conf.conf_data, type, ap);
}

static
int get_conf_int(int def_val, ...)
{
    va_list ap;
    s_exp_data_t *data;

    va_start(ap, def_val);

    data = get_conf_v(S_EXP_TYPE_INTEGER, ap);
    if(data == NULL) {
        data = get_conf_v(S_EXP_TYPE_FLONUM, ap);
    }

    va_end(ap);

    if(data == NULL) {
        return def_val;
    } else {
        if(data->type == S_EXP_TYPE_INTEGER) {
            return data->number.val;
        } else {
            return (int)data->flonum.val;
        }
    }
}

static
s_exp_data_t *get_conf_list(const s_exp_data_t *def_val, ...)
{
    va_list ap;
    s_exp_data_t *data;

    va_start(ap, def_val);
    data = get_conf_v(S_EXP_TYPE_CONS, ap);
    va_end(ap);

    if(data == NULL) {
        return (s_exp_data_t *)def_val;
    } else {
        return data;
    }
}


static
double get_nth_double(const s_exp_data_t *list, int nth, double def_val)
{
    const s_exp_data_t *v;

    v = s_exp_nth(list, nth);
    if(v != NULL) {
        if(v->type == S_EXP_TYPE_FLONUM) {
            return v->flonum.val;
        } else if(v->type == S_EXP_TYPE_INTEGER) {
            return (double)v->number.val;
        }
    }

    return def_val;
}


static
int apply_mode_name(struct mouse_conf *conf, const s_exp_data_t *mode)
{
    int i;
    const s_exp_data_t *p;

    i = 0;
    S_EXP_FOR_EACH(mode, p) {
        if(S_EXP_CAR(p)->type != S_EXP_TYPE_CONS ||
           S_EXP_CAAR(p)->type != S_EXP_TYPE_SYMBOL) {
            return 0;
        }

        conf[i].mode_name = S_EXP_CAAR(p)->symbol.name;

        i += 1;
    }

    return 1;
}

static
struct mouse_conf *get_mode_ptr(const struct mouse_conf *conf, int nconf,
                                wchar_t *name)
{
    int i;

    for(i = 0; i < nconf; i++) {
        if(wcscmp(conf[i].mode_name, name) == 0) {
            return (struct mouse_conf *)&conf[i];
        }
    }

    return NULL;
}


typedef int (*act_proc_t)(struct mouse_action *, const s_exp_data_t *);
struct action_conf_map {
    wchar_t *name;
    act_proc_t proc;
};

static
int apply_act(struct mouse_action *act, const s_exp_data_t *conf,
              struct action_conf_map *map)
{
    int i;

    if(S_EXP_CAR(conf)->type != S_EXP_TYPE_SYMBOL) {
        return 0;
    }

    for(i = 0; map[i].name != NULL; i++) {
        if(wcscmp(S_EXP_CAR(conf)->symbol.name, map[i].name) == 0) {
            return map[i].proc(act, conf);
        }
    }

    return 0;
}

static
int apply_action_none(struct mouse_action *act, const s_exp_data_t *conf)
{
    act->code = MOUSE_ACT_NONE;

    return 1;
}

static
int apply_action_button(struct mouse_action *act, const s_exp_data_t *conf)
{
    if(S_EXP_CDR(conf)->type != S_EXP_TYPE_CONS ||
       S_EXP_CADR(conf)->type != S_EXP_TYPE_INTEGER ||
       S_EXP_CADR(conf)->number.val < 1 ||
       S_EXP_CADR(conf)->number.val > MOUSE_BTN_MAX) {
        return 0;
    }

    act->code = MOUSE_ACT_BUTTON;
    act->conf.button = S_EXP_CADR(conf)->number.val - 1;

    return 1;
}

static
int apply_action_change_normal(struct mouse_action *act,
                               const s_exp_data_t *conf)
{
    struct mouse_conf *mode;

    if(S_EXP_CDR(conf)->type != S_EXP_TYPE_CONS ||
       S_EXP_CADR(conf)->type != S_EXP_TYPE_SYMBOL) {
        return 0;
    }

    mode = get_mode_ptr(ctx.app_conf.normal_conf,
                        ctx.app_conf.normal_conf_num,
                        S_EXP_CADR(conf)->symbol.name);
    if(mode == NULL) {
        return 0;
    }

    act->code = MOUSE_ACT_MODECH;
    act->conf.mode_change.mode = mode;
    act->conf.mode_change.data.mode = MODE_CH_NORMAL;

    return 1;
}

static
int apply_action_change_scroll(struct mouse_action *act,
                               const s_exp_data_t *conf)
{
    struct mouse_conf *mode;

    if(S_EXP_CDR(conf)->type != S_EXP_TYPE_CONS ||
       S_EXP_CADR(conf)->type != S_EXP_TYPE_SYMBOL) {
        return 0;
    }

    mode = get_mode_ptr(ctx.app_conf.scroll_conf,
                        ctx.app_conf.scroll_conf_num,
                        S_EXP_CADR(conf)->symbol.name);
    if(mode == NULL) {
        return 0;
    }

    act->code = MOUSE_ACT_MODECH;
    act->conf.mode_change.mode = mode;
    act->conf.mode_change.data.mode = MODE_CH_SCROLL;

    return 1;
}

static
int apply_action_set_ratio(struct mouse_action *act, const s_exp_data_t *conf)
{
    double xr, yr;

    xr = get_nth_double(conf, 1, 1.0);
    yr = get_nth_double(conf, 2, 1.0);

    act->code = MOUSE_ACT_MODEMSG;
    act->conf.mode_msg.data.mode = MODE_MSG_SET_RATIO;
    act->conf.mode_msg.data.ratio.x = xr;
    act->conf.mode_msg.data.ratio.y = yr;

    return 1;
}

static
int apply_action_mul_ratio(struct mouse_action *act, const s_exp_data_t *conf)
{
    double xr, yr;

    xr = get_nth_double(conf, 1, 1.0);
    yr = get_nth_double(conf, 2, 1.0);

    act->code = MOUSE_ACT_MODEMSG;
    act->conf.mode_msg.data.mode = MODE_MSG_MUL_RATIO;
    act->conf.mode_msg.data.ratio.x = xr;
    act->conf.mode_msg.data.ratio.y = yr;

    return 1;
}

static
int apply_action_wheel_input(struct mouse_action *act,
                             const s_exp_data_t *conf)
{
    act->code = MOUSE_ACT_WHEEL;

    return 1;
}

static
int apply_action_wheel_message(struct mouse_action *act,
                               const s_exp_data_t *conf)
{
    act->code = MOUSE_ACT_WHEELPOST;

    return 1;
}

static
int apply_button_act(struct mouse_action *act, const s_exp_data_t *conf)
{
    static struct action_conf_map map[] = {
        {L"none", apply_action_none},
        {L"button", apply_action_button},
        {L"normal-mode", apply_action_change_normal},
        {L"scroll-mode", apply_action_change_scroll},
        {L"set-scroll-ratio", apply_action_set_ratio},
        {L"mul-scroll-ratio", apply_action_mul_ratio},

        {NULL, NULL}
    };

    return apply_act(act, conf, map);
}

static
int apply_wheel_act(struct mouse_action *act, const s_exp_data_t *conf)
{
    static struct action_conf_map map[] = {
        {L"none", apply_action_none},
        {L"wheel-input", apply_action_wheel_input},
        {L"post-wheel-message", apply_action_wheel_message},

        {NULL, NULL}
    };

    return apply_act(act, conf, map);
}

static
int apply_comm_mode_conf(struct mouse_conf *conf, const s_exp_data_t *mode)
{
    const s_exp_data_t *c;

    {
        int n, m;
        wchar_t btn_name[] = L"button-N";
        wchar_t cmb_name[] = L"button-N+M";
        wchar_t *name;
        struct mouse_action *act[2];

        for(n = 0; n < MOUSE_BTN_MAX; n++) {
            for(m = n; m < MOUSE_BTN_MAX; m++) {
                if(n == m) {
                    btn_name[7] = L'1' + n;

                    name = btn_name;
                    act[0] = &conf->button[n].act;
                } else {
                    cmb_name[7] = L'1' + n;
                    cmb_name[9] = L'1' + m;

                    name = cmb_name;
                    act[0] = &conf->button[n].comb_act[m];
                    act[1] = &conf->button[m].comb_act[n];
                }

                c = s_exp_massq(mode, S_EXP_TYPE_CONS, name, NULL);
                if(c == NULL || S_EXP_CAR(c)->type != S_EXP_TYPE_CONS ||
                   apply_button_act(act[0], S_EXP_CAR(c)) == 0) {
                    if(n == m) {
                        act[0]->code = MOUSE_ACT_BUTTON;
                        act[0]->conf.button = n;
                    } else {
                        continue;
                    }
                }

                if(n != m) {
                    conf->button[n].flags |= MOUSE_BTN_CONF_ENABLE_COMB;
                    conf->button[m].flags |= MOUSE_BTN_CONF_ENABLE_COMB;

                    memcpy(act[1], act[0], sizeof(struct mouse_action));
                }
            }
        }
    }

    c = s_exp_massq(mode, S_EXP_TYPE_CONS, L"wheel", NULL);
    if(c != NULL && S_EXP_CAR(c)->type == S_EXP_TYPE_CONS) {
        apply_wheel_act(&conf->wheel_act, S_EXP_CAR(c));
    }

    return 1;
}

static
int apply_mode_conf(void)
{
    s_exp_data_t *normal_mode, *scroll_mode;

    normal_mode = get_conf_list(S_EXP_NIL, L"normal-mode", NULL);
    scroll_mode = get_conf_list(S_EXP_NIL, L"scroll-mode", NULL);

    ctx.app_conf.normal_conf_num = s_exp_length(normal_mode);
    ctx.app_conf.scroll_conf_num = s_exp_length(scroll_mode);

    if(ctx.app_conf.normal_conf_num == 0) {
        ctx.app_conf.normal_conf_num = 1;
    }

    ctx.app_conf.normal_conf =
        (struct mouse_conf *)malloc(sizeof(struct mouse_conf) *
                                    ctx.app_conf.normal_conf_num);
    ctx.app_conf.scroll_conf =
        (struct mouse_conf *)malloc(sizeof(struct mouse_conf) *
                                    ctx.app_conf.scroll_conf_num);
    if(ctx.app_conf.normal_conf == NULL ||
       ctx.app_conf.scroll_conf == NULL) {
        goto fail_end;
    }

    memset(ctx.app_conf.normal_conf, 0,
           sizeof(struct mouse_conf) * ctx.app_conf.normal_conf_num);
    memset(ctx.app_conf.scroll_conf, 0,
           sizeof(struct mouse_conf) * ctx.app_conf.scroll_conf_num);

    if(normal_mode != S_EXP_NIL) {
        int i;
        s_exp_data_t *p;

        if(apply_mode_name(ctx.app_conf.normal_conf, normal_mode) == 0 ||
           apply_mode_name(ctx.app_conf.scroll_conf, scroll_mode) == 0) {
            goto fail_end;
        }

        i = 0;
        S_EXP_FOR_EACH(normal_mode, p) {
            apply_comm_mode_conf(&ctx.app_conf.normal_conf[i], S_EXP_CDAR(p));

            ctx.app_conf.normal_conf[i].move_act.code = MOUSE_ACT_MOVE;

            i += 1;
        }

        i = 0;
        S_EXP_FOR_EACH(scroll_mode, p) {
            s_exp_data_t *sr;

            apply_comm_mode_conf(&ctx.app_conf.scroll_conf[i], S_EXP_CDAR(p));

            ctx.app_conf.scroll_conf[i].move_act.code = MOUSE_ACT_MODEMSG;
            ctx.app_conf.scroll_conf[i].move_act.conf.mode_msg.data.mode =
                MODE_MSG_SCROLL;

            sr = s_exp_massq(S_EXP_CDAR(p), S_EXP_TYPE_CONS,
                             L"scroll-ratio", NULL);
            if(sr == NULL) {
                sr = S_EXP_NIL;
            }
            ctx.app_conf.scroll_conf[i].scroll_mode.x_ratio =
                get_nth_double(sr, 0, 1.0);
            ctx.app_conf.scroll_conf[i].scroll_mode.y_ratio =
                get_nth_double(sr, 1, 1.0);

            i += 1;
        }
    } else {
        int i;
        struct mouse_conf *conf;

        conf = &ctx.app_conf.normal_conf[0];

        conf->mode_name = L"default";

        for(i = 0; i < MOUSE_BTN_MAX; i++) {
            conf->button[i].flags = 0;
            conf->button[i].act.code = MOUSE_ACT_BUTTON;
            conf->button[i].act.conf.button = i;
        }
        conf->wheel_act.code = MOUSE_ACT_WHEEL;
        conf->move_act.code = MOUSE_ACT_MOVE;
    }

    return 1;

  fail_end:
    if(ctx.app_conf.normal_conf != NULL) free(ctx.app_conf.normal_conf);
    if(ctx.app_conf.scroll_conf != NULL) free(ctx.app_conf.scroll_conf);

    return 0;
}


static
int apply_scroll_operator(void)
{
    int i, n_builtin_sop;

    /* number of builtin scroll operators */
    for(i = 0; builtin_scroll_op[i].name != NULL; i++) ;
    n_builtin_sop = i;

    /* allocate */
    ctx.app_conf.scroll_operator_num = n_builtin_sop;
    ctx.app_conf.scroll_operator_conf =
        (struct scroll_operator_conf *)
        malloc(sizeof(struct scroll_operator_conf) *
               ctx.app_conf.scroll_operator_num);
    if(ctx.app_conf.scroll_operator_conf == NULL) {
        return 0;
    }

    memset(ctx.app_conf.scroll_operator_conf, 0,
           sizeof(struct scroll_operator_conf) *
           ctx.app_conf.scroll_operator_num);

    /* setup builtin scroll operators */
    for(i = 0; i < n_builtin_sop; i++) {
        ctx.app_conf.scroll_operator_conf[i].name = builtin_scroll_op[i].name;

        if(builtin_scroll_op[i].get_op_proc(
               &ctx.app_conf.scroll_operator_conf[i].proc,
               SCROLL_OP_API_VERSION) == 0) {
            return 0;
        }
    }

    /* fill configuration for each operators */
    for(i = 0; i < ctx.app_conf.scroll_operator_num; i++) {
        ctx.app_conf.scroll_operator_conf[i].conf =
            get_conf_list(S_EXP_NIL,
                          L"scroll-operator",
                          ctx.app_conf.scroll_operator_conf[i].name,
                          NULL);
    }

    return 1;
}


static
int get_class_title_regexp(const s_exp_data_t *e,
                           wchar_t **class_re, wchar_t **title_re,
                           int *class_or_title)
{
    if(e->type == S_EXP_TYPE_STRING) {
        *class_re = e->string.str;
        *title_re = NULL;
        *class_or_title = 1;
        return 1;
    } else if(e->type == S_EXP_TYPE_CONS) {
        if(S_EXP_CAR(e)->type != S_EXP_TYPE_SYMBOL) {
            return 0;
        }

        if(wcscmp(S_EXP_CAR(e)->symbol.name, L"class") == 0) {
            if(S_EXP_CDR(e)->type != S_EXP_TYPE_CONS ||
               S_EXP_CADR(e)->type != S_EXP_TYPE_STRING ||
               S_EXP_CDDR(e) != S_EXP_NIL) {
                return 0;
            }

            *class_re = S_EXP_CADR(e)->string.str;
            *title_re = NULL;
            *class_or_title = 1;
            return 1;
        }

        if(wcscmp(S_EXP_CAR(e)->symbol.name, L"title") == 0) {
            if(S_EXP_CDR(e)->type != S_EXP_TYPE_CONS ||
               S_EXP_CADR(e)->type != S_EXP_TYPE_STRING ||
               S_EXP_CDDR(e) != S_EXP_NIL) {
                return 0;
            }

            *class_re = NULL;
            *title_re = S_EXP_CADR(e)->string.str;
            *class_or_title = 1;
            return 1;
        }

        if(wcscmp(S_EXP_CAR(e)->symbol.name, L"or") == 0 ||
           wcscmp(S_EXP_CAR(e)->symbol.name, L"and") == 0) {
            wchar_t *cr1, *tr1, *cr2, *tr2;
            int o1, o2;

            if(S_EXP_CDR(e)->type != S_EXP_TYPE_CONS ||
               S_EXP_CDDR(e)->type != S_EXP_TYPE_CONS ||
               S_EXP_CDDDR(e) != S_EXP_NIL) {
                return 0;
            }

            if(get_class_title_regexp(S_EXP_CADR(e), &cr1, &tr1, &o1) == 0 ||
               get_class_title_regexp(S_EXP_CADDR(e), &cr2, &tr2, &o2) == 0) {
                return 0;
            }

            if((cr1 != NULL && cr2 != NULL) ||
               (tr1 != NULL && tr2 != NULL) ||
               (cr1 == NULL && cr2 == NULL && tr1 == NULL && tr2 == NULL)) {
                return 0;
            }

            *class_re = (cr1 != NULL ? cr1 : cr2);
            *title_re = (tr1 != NULL ? tr1 : tr2);
            *class_or_title = (wcscmp(S_EXP_CAR(e)->symbol.name, L"or") == 0);
            return 1;
        }
    }

    return 0;
}

static
int apply_scroll_window(void)
{
    s_exp_data_t *list, *p;
    int i, j;

    /* number of scroll window configuration */
    list = get_conf_list(S_EXP_NIL, L"scroll-window", NULL);

    /* allocate */
    ctx.app_conf.window_conf_num = s_exp_length(list);
    ctx.app_conf.window_conf =
        (struct scroll_window_conf *)
        malloc(sizeof(struct scroll_window_conf) *
               ctx.app_conf.window_conf_num);
    if(ctx.app_conf.window_conf == NULL) {
        return 0;
    }

    /* for each window configuration */
    i = 0;
    S_EXP_FOR_EACH(list, p) {
        s_exp_data_t *e, *op_arg;
        wchar_t *class_re, *title_re, *op_name;
        int class_or_title;
        struct scroll_operator_conf *op;

        e = S_EXP_CAR(p);

        if(e->type != S_EXP_TYPE_CONS) {
            continue;
        }

        /* class/title regexp */
        if(get_class_title_regexp(S_EXP_CAR(e),
                                  &class_re, &title_re,
                                  &class_or_title) == 0) {
            continue;
        }

        /* operator name and args */
        if(S_EXP_CDR(e)->type != S_EXP_TYPE_CONS ||
           S_EXP_CADR(e)->type != S_EXP_TYPE_CONS ||
           S_EXP_CAADR(e)->type != S_EXP_TYPE_SYMBOL) {
            continue;
        }

        op_name = S_EXP_CAADR(e)->symbol.name;
        op_arg = S_EXP_CDADR(e);

        /* operator */
        op = NULL;
        for(j = 0; j < ctx.app_conf.scroll_operator_num; j++) {
            if(wcscmp(op_name,
                      ctx.app_conf.scroll_operator_conf[j].name) == 0) {
                op = &ctx.app_conf.scroll_operator_conf[j];
                break;
            }
        }
        if(op == NULL) {
            continue;
        }

        /* fill */
        ctx.app_conf.window_conf[i].class_regexp = class_re;
        ctx.app_conf.window_conf[i].title_regexp = title_re;
        ctx.app_conf.window_conf[i].class_or_title = class_or_title;
        ctx.app_conf.window_conf[i].op = op;
        ctx.app_conf.window_conf[i].args = op_arg;
        ctx.app_conf.window_conf[i].regexp = e;

        i += 1;
    }

    ctx.app_conf.window_conf_num = i;

    return 1;
}


int apply_setting(void)
{
    int ret;

    /* global setting */
    ctx.app_conf.comb_time = get_conf_int(300,
                                          L"global", L"combination-time",
                                          NULL);

    /* normal, scroll mode */
    ret = apply_mode_conf();
    if(ret == 0) {
        return 0;
    }

    ctx.app_conf.cur_conf = &ctx.app_conf.normal_conf[0];

    /* scroll operator */
    ret = apply_scroll_operator();
    if(ret == 0) {
        return 0;
    }

    /* scroll window */
    ret = apply_scroll_window();
    if(ret == 0) {
        return 0;
    }

    return 1;
}
