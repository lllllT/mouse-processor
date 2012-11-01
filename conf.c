/*
 * conf.h  -- configuration
 *
 */

#include "main.h"
#include "util.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


#define HOME_ENV L"%HOME%\\"
#define HOMEPATH_ENV L"%HOMEDRIVE%%HOMEPATH%\\"
#define USERPROFILE_ENV L"%USERPROFILE%\\"

static wchar_t *rc_names[] = { L".mprc", L"dot.mprc", L"default.mprc" };

static int include_depth;


static
LPWSTR expand_env_str(LPCWSTR str)
{
    DWORD size;
    LPWSTR s;

    size = ExpandEnvironmentStringsW(str, NULL, 0);
    if(size == 0) {
        return NULL;
    }
    size += 1;

    s = (LPWSTR)malloc(sizeof(WCHAR) * size);
    if(s == NULL) {
        return NULL;
    }
    memset(s, 0, sizeof(WCHAR) * size);

    ExpandEnvironmentStringsW(str, s, size);

    return s;
}

static
LPWSTR wcscat_dup(LPCWSTR s1, LPCWSTR s2)
{
    DWORD size = 1;
    LPWSTR s;

    size += wcslen(s1);
    size += wcslen(s2);

    s = (LPWSTR)malloc(sizeof(WCHAR) * size);
    if(s == NULL) {
        return NULL;
    }

    memset(s, 0, sizeof(WCHAR) * size);

    wcscat(s, s1);
    wcscat(s, s2);

    return s;
}

static
LPWSTR get_home_path(LPCWSTR rel_path)
{
    return wcscat_dup(HOME_ENV, rel_path);
}

static
LPWSTR get_home_drivepath(LPCWSTR rel_path)
{
    return wcscat_dup(HOMEPATH_ENV, rel_path);
}

static
LPWSTR get_userprofile_path(LPCWSTR rel_path)
{
    return wcscat_dup(USERPROFILE_ENV, rel_path);
}

static
LPWSTR get_cwd_path(LPCWSTR rel_path)
{
    DWORD size;
    LPWSTR path;

    size = GetCurrentDirectory(0, NULL);
    if(size == 0) {
        return NULL;
    }
    size += 1 + wcslen(rel_path);

    path = (LPWSTR)malloc(sizeof(WCHAR) * size);
    if(path == NULL) {
        return NULL;
    }
    memset(path, 0, sizeof(WCHAR) * size);

    GetCurrentDirectory(size, path);

    wcscat(path, L"\\");
    wcscat(path, rel_path);

    return path;
}

static
LPWSTR get_module_path(LPCWSTR rel_path)
{
    DWORD size;
    LPWSTR path;
    WCHAR buf[512];

    size = GetModuleFileName(NULL, buf, 512);
    if(size == 0) {
        return NULL;
    }
    size += 1 + wcslen(rel_path);

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

    wcscat(path, L"\\");
    wcscat(path, rel_path);

    return path;
}


static s_exp_data_t *load_conf(LPCWSTR conf_file, s_exp_data_t *base_data);

static
s_exp_data_t *load_file_conf(LPCWSTR path)
{
    char *u8s_path = NULL;
    FILE *fp = NULL;
    s_exp_read_context_t *rc = NULL;
    s_exp_data_t *data = NULL;

    log_printf(LOG_LEVEL_DEBUG, L"Reading config file: %ls\n", path);

    u8s_path = u8s_dup_from_wcs(path);
    if(u8s_path == NULL) {
        log_printf(LOG_LEVEL_NOTIFY,
                   L"Failed to read: %ls: %hs\n", path, strerror(errno));
        goto end;
    }

    fp = _wfopen(path, L"r");
    if(fp == NULL) {
        log_printf(LOG_LEVEL_DEBUG,
                   L"Failed to read: %ls: %hs\n", path, strerror(errno));
        goto end;
    }

    rc = open_s_exp_read_context_f(fp, u8s_path);
    if(rc == NULL) {
        log_printf(LOG_LEVEL_NOTIFY,
                   L"Failed to read: %ls: %hs\n", path, strerror(errno));
        goto end;
    }

    data = read_all_s_exp(rc);

    log_printf(LOG_LEVEL_NOTIFY, L"Read config file completed: %ls\n", path);

  end:
    if(u8s_path != NULL) free(u8s_path);
    if(fp != NULL) fclose(fp);
    if(rc != NULL) close_s_exp_read_context(rc);

    return data;
}

static
s_exp_data_t *merge_conf_data(s_exp_data_t *data, s_exp_data_t *part)
{
    if(part->type == S_EXP_TYPE_CONS &&
       S_EXP_CAR(part)->type == S_EXP_TYPE_SYMBOL) {
        /* part is pair and it's car is symbol: merge */
        s_exp_data_t *sym = S_EXP_CAR(part);
        s_exp_data_t *p = s_exp_assq_get(data, sym->symbol.name);

        if(p == NULL) {
            s_exp_data_t *d;

            sym = s_exp_intern(sym->symbol.name);
            if(S_EXP_ERROR(sym)) {
                free_s_exp(data);
                free_s_exp(part);
                return sym;
            }

            p = s_exp_cons(sym, S_EXP_NIL);
            if(S_EXP_ERROR(p)) {
                free_s_exp(data);
                free_s_exp(part);
                return p;
            }

            d = s_exp_cons(p, data);
            if(S_EXP_ERROR(d)) {
                free_s_exp(p);
                free_s_exp(data);
                free_s_exp(part);
                return d;
            }

            data = d;
        }

        if(S_EXP_CDR(part)->type != S_EXP_TYPE_CONS ||
           S_EXP_CADR(part)->type != S_EXP_TYPE_CONS) {
            S_EXP_CDR(p) = S_EXP_CDR(part);
            S_EXP_CDR(part) = S_EXP_NIL;
        } else {
            s_exp_data_t *pp;
            S_EXP_FOR_EACH(S_EXP_CDR(part), pp) {
                s_exp_data_t *d = merge_conf_data(S_EXP_CDR(p),
                                                  S_EXP_CAR(pp));
                if(S_EXP_ERROR(d)) {
                    S_EXP_CDR(p) = S_EXP_NIL;
                    S_EXP_CAR(pp) = S_EXP_NIL;
                    free_s_exp(data);
                    free_s_exp(part);
                    return d;
                }

                S_EXP_CDR(p) = d;
                S_EXP_CAR(pp) = S_EXP_NIL;
            }
        }
        free_s_exp(part);

        return data;
    } else {
        /* prepend to data */
        s_exp_data_t *d = s_exp_cons(part, data);
        if(S_EXP_ERROR(d)) {
            free_s_exp(data);
            free_s_exp(part);
        }

        return d;
    }
}

static
s_exp_data_t *merge_conf_data_with_include(s_exp_data_t *data1,
                                           s_exp_data_t *data2)
{
    s_exp_data_t *p;

    S_EXP_FOR_EACH(data2, p) {
        s_exp_data_t *l = S_EXP_CAR(p);

        if(l->type == S_EXP_TYPE_CONS &&
           S_EXP_CAR(l)->type == S_EXP_TYPE_SYMBOL &&
           wcscmp(S_EXP_CAR(l)->symbol.name, L"include") == 0) {
            /* (include ...) */
            s_exp_data_t *pp;
            S_EXP_FOR_EACH(S_EXP_CDR(l), pp) {
                if(S_EXP_CAR(pp)->type != S_EXP_TYPE_STRING) {
                    log_printf(LOG_LEVEL_ERROR, L"Invalid include argument: ");
                    log_print_s_exp(LOG_LEVEL_ERROR, S_EXP_CAR(pp), 1);
                    continue;
                }

                {
                    include_depth += 1;
                    s_exp_data_t *d = load_conf(S_EXP_CAR(pp)->string.str,
                                                data1);
                    include_depth -= 1;
                    if(d == NULL) {
                        log_printf(LOG_LEVEL_ERROR,
                                   L"include: config file not found: ");
                        log_print_s_exp(LOG_LEVEL_ERROR, S_EXP_CAR(pp), 1);
                        continue;
                    }

                    data1 = d;
                }
            }
        } else {
            /* merge */
            data1 = merge_conf_data(data1, l);
            if(S_EXP_ERROR(data1)) {
                free_s_exp(data2);
                return NULL;
            }

            S_EXP_CAR(p) = S_EXP_NIL;
        }
    }

    free_s_exp(data2);

    return data1;
}

static
s_exp_data_t *load_merged_file_conf(LPCWSTR path, s_exp_data_t *data)
{
    LPWSTR p = expand_env_str(path);
    if(p == NULL) {
        return NULL;
    }

    s_exp_data_t *d = load_file_conf(p);
    free(p);
    if(d == NULL) {
        return NULL;
    }

    return merge_conf_data_with_include(data, d);
}

static
s_exp_data_t *load_conf(LPCWSTR conf_file, s_exp_data_t *base_data)
{
    LPWSTR fname, path;
    s_exp_data_t *data;

    if(include_depth >= 20) {
        log_printf(LOG_LEVEL_WARNING, L"too deep include: %ls\n", conf_file);
        return base_data;
    }

    if(conf_file == NULL) {
        int i;
        for(i = 0; i < sizeof(rc_names) / sizeof(rc_names[0]); i++) {
            data = load_conf(rc_names[i], base_data);
            if(data != NULL) {
                return data;
            }
        }

        return NULL;
    }

    fname = expand_env_str(conf_file);
    if(fname != NULL &&
       (fname[0] == L'/' ||
        fname[0] == L'\\' ||
        (((fname[0] >= L'a' && fname[0] <= L'z') ||
          (fname[0] >= L'A' && fname[0] <= L'Z')) &&
         fname[1] == L':'))) {
        free(fname);
        /* abs path */
        return load_merged_file_conf(conf_file, base_data);
    }
    free(fname);

    /* try to read from "%HOME%/..." */
    if((path = get_home_path(conf_file)) != NULL) {
        data = load_merged_file_conf(path, base_data);
        free(path);

        if(data != NULL) {
            return data;
        }
    }

    /* try to read from "%HOMEDRIVE%%HOMEPATH%/..." */
    if((path = get_home_drivepath(conf_file)) != NULL) {
        data = load_merged_file_conf(path, base_data);
        free(path);

        if(data != NULL) {
            return data;
        }
    }

    /* try to read from "%USERPROFILE%/..." */
    if((path = get_userprofile_path(conf_file)) != NULL) {
        data = load_merged_file_conf(path, base_data);
        free(path);

        if(data != NULL) {
            return data;
        }
    }

    /* try to read from current direcotry */
    if((path = get_cwd_path(conf_file)) != NULL) {
        data = load_merged_file_conf(path, base_data);
        free(path);

        if(data != NULL) {
            return data;
        }
    }

    /* try to read from "<module-path>/..." */
    if((path = get_module_path(conf_file)) != NULL) {
        data = load_merged_file_conf(path, base_data);
        free(path);

        if(data != NULL) {
            return data;
        }
    }

    return NULL;
}


static
s_exp_data_t *get_conf_v(struct app_setting *conf, int type, va_list ap)
{
    return s_exp_massq_v(conf->conf_data, type, ap);
}

static
s_exp_data_t *get_conf(struct app_setting *conf, int type, ...)
{
    va_list ap;
    s_exp_data_t *data;

    va_start(ap, type);
    data = get_conf_v(conf, type, ap);
    va_end(ap);

    return data;
}

static
int get_conf_int(struct app_setting *conf, int def_val, ...)
{
    va_list ap;
    s_exp_data_t *data;

    va_start(ap, def_val);

    data = get_conf_v(conf, S_EXP_TYPE_INTEGER, ap);
    if(data == NULL) {
        data = get_conf_v(conf, S_EXP_TYPE_FLONUM, ap);
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
s_exp_data_t *get_conf_list(struct app_setting *conf,
                            const s_exp_data_t *def_val, ...)
{
    va_list ap;
    s_exp_data_t *data;

    va_start(ap, def_val);
    data = get_conf_v(conf, S_EXP_TYPE_CONS, ap);
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
int get_nth_int(const s_exp_data_t *list, int nth, int def_val)
{
    const s_exp_data_t *v;

    v = s_exp_nth(list, nth);
    if(v != NULL) {
        if(v->type == S_EXP_TYPE_FLONUM) {
            return (int)v->flonum.val;
        } else if(v->type == S_EXP_TYPE_INTEGER) {
            return v->number.val;
        }
    }

    return def_val;
}

static
LPWSTR get_nth_str(const s_exp_data_t *list, int nth, LPCWSTR def_val)
{
    const s_exp_data_t *v;

    v = s_exp_nth(list, nth);
    if(v != NULL) {
        if(v->type == S_EXP_TYPE_STRING) {
            return v->string.str;
        }
    }

    return (LPWSTR)def_val;
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
            log_printf(LOG_LEVEL_ERROR, L"Invalid mode format: ");
            log_print_s_exp(LOG_LEVEL_ERROR, S_EXP_CAR(p), 1);
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


typedef int (*act_proc_t)(struct mouse_action *,
                          const struct app_setting *, const s_exp_data_t *);
struct action_conf_map {
    wchar_t *name;
    act_proc_t proc;
};

static
int apply_act(struct mouse_action *act,
              const struct app_setting *app_conf, const s_exp_data_t *conf,
              struct action_conf_map *map)
{
    int i;

    if(S_EXP_CAR(conf)->type != S_EXP_TYPE_SYMBOL) {
        return 0;
    }

    for(i = 0; map[i].name != NULL; i++) {
        if(wcscmp(S_EXP_CAR(conf)->symbol.name, map[i].name) == 0) {
            return map[i].proc(act, app_conf, conf);
        }
    }
    log_printf(LOG_LEVEL_WARNING, L"Invalid action format: unknown action: ");
    log_print_s_exp(LOG_LEVEL_WARNING, conf, 1);

    return 0;
}

static
int apply_action_nothing(struct mouse_action *act,
                         const struct app_setting *app_conf,
                         const s_exp_data_t *conf)
{
    act->code = MOUSE_ACT_NOTHING;

    return 1;
}

static
int apply_action_button_d(struct mouse_action *act,
                          const struct app_setting *app_conf,
                          const s_exp_data_t *conf)
{
    if(S_EXP_CDR(conf)->type != S_EXP_TYPE_CONS ||
       S_EXP_CADR(conf)->type != S_EXP_TYPE_INTEGER ||
       S_EXP_CADR(conf)->number.val < 1 ||
       S_EXP_CADR(conf)->number.val > MOUSE_BTN_MAX) {
        log_printf(LOG_LEVEL_WARNING,
                   L"Invalid action format: button-d action: ");
        log_print_s_exp(LOG_LEVEL_WARNING, conf, 1);
        return 0;
    }

    act->code = MOUSE_ACT_BUTTON_D;
    act->conf.button = S_EXP_CADR(conf)->number.val - 1;

    return 1;
}

static
int apply_action_button_u(struct mouse_action *act,
                          const struct app_setting *app_conf,
                          const s_exp_data_t *conf)
{
    if(S_EXP_CDR(conf)->type != S_EXP_TYPE_CONS ||
       S_EXP_CADR(conf)->type != S_EXP_TYPE_INTEGER ||
       S_EXP_CADR(conf)->number.val < 1 ||
       S_EXP_CADR(conf)->number.val > MOUSE_BTN_MAX) {
        log_printf(LOG_LEVEL_WARNING,
                   L"Invalid action format: button-u action: ");
        log_print_s_exp(LOG_LEVEL_WARNING, conf, 1);
        return 0;
    }

    act->code = MOUSE_ACT_BUTTON_U;
    act->conf.button = S_EXP_CADR(conf)->number.val - 1;

    return 1;
}

static
int apply_action_change_normal(struct mouse_action *act,
                               const struct app_setting *app_conf,
                               const s_exp_data_t *conf)
{
    struct mouse_conf *mode;

    if(S_EXP_CDR(conf) == S_EXP_NIL) {
        mode = app_conf->initial_conf;
    } else {
        if(S_EXP_CDR(conf)->type != S_EXP_TYPE_CONS ||
           S_EXP_CADR(conf)->type != S_EXP_TYPE_SYMBOL) {
            log_printf(LOG_LEVEL_WARNING,
                       L"Invalid action format: normal-mode action: ");
            log_print_s_exp(LOG_LEVEL_WARNING, conf, 1);
            return 0;
        }

        mode = get_mode_ptr(app_conf->normal_conf,
                            app_conf->normal_conf_num,
                            S_EXP_CADR(conf)->symbol.name);
        if(mode == NULL) {
            log_printf(LOG_LEVEL_WARNING,
                       L"Invalid action format: "
                       L"specified normal-mode not found: ");
            log_print_s_exp(LOG_LEVEL_WARNING, conf, 1);
            return 0;
        }
    }

    act->code = MOUSE_ACT_MODECH;
    act->conf.mode_change.mode = mode;
    act->conf.mode_change.data.mode = MODE_CH_NORMAL;

    return 1;
}

static
int apply_action_change_scroll(struct mouse_action *act,
                               const struct app_setting *app_conf,
                               const s_exp_data_t *conf)
{
    struct mouse_conf *mode;

    if(S_EXP_CDR(conf)->type != S_EXP_TYPE_CONS ||
       S_EXP_CADR(conf)->type != S_EXP_TYPE_SYMBOL) {
        log_printf(LOG_LEVEL_WARNING,
                   L"Invalid action format: scroll-mode action: ");
        log_print_s_exp(LOG_LEVEL_WARNING, conf, 1);
        return 0;
    }

    mode = get_mode_ptr(app_conf->scroll_conf,
                        app_conf->scroll_conf_num,
                        S_EXP_CADR(conf)->symbol.name);
    if(mode == NULL) {
        log_printf(LOG_LEVEL_WARNING,
                   L"Invalid action format: "
                   L"specified scroll-mode not found: ");
        log_print_s_exp(LOG_LEVEL_WARNING, conf, 1);
        return 0;
    }

    act->code = MOUSE_ACT_MODECH;
    act->conf.mode_change.mode = mode;
    act->conf.mode_change.data.mode = MODE_CH_SCROLL;

    return 1;
}

static
int apply_action_set_ratio(struct mouse_action *act,
                           const struct app_setting *app_conf,
                           const s_exp_data_t *conf)
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
int apply_action_mul_ratio(struct mouse_action *act,
                           const struct app_setting *app_conf,
                           const s_exp_data_t *conf)
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
                             const struct app_setting *app_conf,
                             const s_exp_data_t *conf)
{
    act->code = MOUSE_ACT_WHEEL;

    return 1;
}

static
int apply_action_wheel_message(struct mouse_action *act,
                               const struct app_setting *app_conf,
                               const s_exp_data_t *conf)
{
    act->code = MOUSE_ACT_WHEELPOST;

    return 1;
}

static
int apply_button_act(struct mouse_action *act,
                     const struct app_setting *app_conf,
                     const s_exp_data_t *conf)
{
    static struct action_conf_map map[] = {
        {L"nothing", apply_action_nothing},
        {L"button-d", apply_action_button_d},
        {L"button-u", apply_action_button_u},
        {L"normal-mode", apply_action_change_normal},
        {L"scroll-mode", apply_action_change_scroll},
        {L"set-scroll-ratio", apply_action_set_ratio},
        {L"mul-scroll-ratio", apply_action_mul_ratio},

        {NULL, NULL}
    };

    return apply_act(act, app_conf, conf, map);
}

static
int apply_wheel_act(struct mouse_action *act,
                    const struct app_setting *app_conf,
                    const s_exp_data_t *conf)
{
    static struct action_conf_map map[] = {
        {L"nothing", apply_action_nothing},
        {L"wheel-input", apply_action_wheel_input},
        {L"post-wheel-message", apply_action_wheel_message},

        {NULL, NULL}
    };

    return apply_act(act, app_conf, conf, map);
}

static
int apply_comm_mode_conf(struct mouse_conf *conf,
                         const struct app_setting *app_conf,
                         const s_exp_data_t *mode,
                         BOOL set_default_p)
{
    const s_exp_data_t *c;

    {
        int n, m, s;
        wchar_t name[64];
        struct mouse_action *act[2];

        for(n = 0; n < MOUSE_BTN_MAX; n++) {
            for(m = n; m < MOUSE_BTN_MAX; m++) {
                for(s = 0; s < 2; s++) {
                    if(n == m) {
                        wsprintfW(name, L"button-%hs-%d",
                                  (s == 0 ? "d" : "u"), n + 1);

                        act[0] = (s == 0 ?
                                  &conf->button[n].d_act :
                                  &conf->button[n].u_act);
                    } else {
                        wsprintfW(name, L"button-%hs-%d+%d",
                                  (s == 0 ? "d" : "u"), n + 1, m + 1);

                        act[0] = (s == 0 ?
                                  &conf->button[n].comb_d_act[m] :
                                  &conf->button[n].comb_u_act[m]);
                        act[1] = (s == 0 ?
                                  &conf->button[m].comb_d_act[n] :
                                  &conf->button[m].comb_u_act[n]);
                    }

                    c = s_exp_assq(mode, name);
                    if(c == NULL ||
                       c->type != S_EXP_TYPE_CONS ||
                       apply_button_act(act[0], app_conf, c) == 0) {
                        if(c != NULL) {
                            log_printf(
                                LOG_LEVEL_WARNING,
                                L"Invalid action format: %ls: ", name);
                            log_print_s_exp(LOG_LEVEL_WARNING, c, 1);
                        }

                        if(n == m && set_default_p) {
                            act[0]->code =
                                (s == 0 ?
                                 MOUSE_ACT_BUTTON_D : MOUSE_ACT_BUTTON_U);
                            act[0]->conf.button = n;
                        } else {
                            continue;
                        }
                    }

                    if(n != m && act[0]->code != MOUSE_ACT_NOTHING) {
                        conf->button[n].flags |= MOUSE_BTN_CONF_ENABLE_COMB;
                        conf->button[m].flags |= MOUSE_BTN_CONF_ENABLE_COMB;

                        memcpy(act[1], act[0], sizeof(struct mouse_action));
                    }
                }
            }
        }
    }

    c = s_exp_massq(mode, S_EXP_TYPE_CONS, L"wheel", NULL);
    if(c != NULL && c->type == S_EXP_TYPE_CONS) {
        apply_wheel_act(&conf->wheel_act, app_conf, c);
    }

    return 1;
}

static
int apply_mode_conf(struct app_setting *app_conf)
{
    s_exp_data_t *normal_mode, *scroll_mode;

    normal_mode = get_conf_list(app_conf, S_EXP_NIL, L"normal-mode", NULL);
    scroll_mode = get_conf_list(app_conf, S_EXP_NIL, L"scroll-mode", NULL);

    app_conf->normal_conf_num = s_exp_length(normal_mode);
    app_conf->scroll_conf_num = s_exp_length(scroll_mode);

    if(app_conf->normal_conf_num == 0) {
        app_conf->normal_conf_num = 1;
    }

    app_conf->normal_conf =
        (struct mouse_conf *)malloc(sizeof(struct mouse_conf) *
                                    app_conf->normal_conf_num);
    app_conf->scroll_conf =
        (struct mouse_conf *)malloc(sizeof(struct mouse_conf) *
                                    app_conf->scroll_conf_num);
    if(app_conf->normal_conf == NULL ||
       app_conf->scroll_conf == NULL) {
        goto fail_end;
    }

    memset(app_conf->normal_conf, 0,
           sizeof(struct mouse_conf) * app_conf->normal_conf_num);
    memset(app_conf->scroll_conf, 0,
           sizeof(struct mouse_conf) * app_conf->scroll_conf_num);

    if(normal_mode != S_EXP_NIL) {
        int i;
        s_exp_data_t *p;

        if(apply_mode_name(app_conf->normal_conf, normal_mode) == 0 ||
           apply_mode_name(app_conf->scroll_conf, scroll_mode) == 0) {
            goto fail_end;
        }

        {
            s_exp_data_t *t;

            t = get_conf(app_conf, S_EXP_TYPE_SYMBOL,
                         L"global", L"initial-mode", NULL);

            app_conf->initial_conf = &app_conf->normal_conf[0];
            if(t != NULL) {
                struct mouse_conf *m;

                m = get_mode_ptr(app_conf->normal_conf,
                                 app_conf->normal_conf_num,
                                 t->symbol.name);

                if(m == NULL) {
                    log_printf(LOG_LEVEL_WARNING,
                               L"Invalid initial-mode name: "
                               L"specified normal-mode not found: ");
                    log_print_s_exp(LOG_LEVEL_WARNING, t, 1);
                } else {
                    app_conf->initial_conf = m;
                }
            }
        }

        i = 0;
        S_EXP_FOR_EACH(normal_mode, p) {
            apply_comm_mode_conf(&app_conf->normal_conf[i],
                                 app_conf, S_EXP_CDAR(p),
                                 TRUE);

            app_conf->normal_conf[i].move_act.code = MOUSE_ACT_MOVE;

            i += 1;
        }

        i = 0;
        S_EXP_FOR_EACH(scroll_mode, p) {
            s_exp_data_t *sr;

            apply_comm_mode_conf(&app_conf->scroll_conf[i],
                                 app_conf, S_EXP_CDAR(p),
                                 FALSE);

            app_conf->scroll_conf[i].move_act.code = MOUSE_ACT_MODEMSG;
            app_conf->scroll_conf[i].move_act.conf.mode_msg.data.mode =
                MODE_MSG_SCROLL;

            sr = s_exp_massq(S_EXP_CDAR(p), S_EXP_TYPE_CONS,
                             L"scroll-ratio", NULL);
            if(sr == NULL) {
                sr = S_EXP_NIL;
            }
            app_conf->scroll_conf[i].scroll_mode.x_ratio =
                get_nth_double(sr, 0, 1.0);
            app_conf->scroll_conf[i].scroll_mode.y_ratio =
                get_nth_double(sr, 1, 1.0);

            i += 1;
        }
    } else {
        int i;
        struct mouse_conf *conf;

        conf = &app_conf->normal_conf[0];

        conf->mode_name = L"default";

        for(i = 0; i < MOUSE_BTN_MAX; i++) {
            conf->button[i].flags = 0;
            conf->button[i].d_act.code = MOUSE_ACT_BUTTON_D;
            conf->button[i].d_act.conf.button = i;
            conf->button[i].u_act.code = MOUSE_ACT_BUTTON_U;
            conf->button[i].u_act.conf.button = i;
        }
        conf->wheel_act.code = MOUSE_ACT_WHEEL;
        conf->move_act.code = MOUSE_ACT_MOVE;

        app_conf->initial_conf = conf;
    }

    return 1;

  fail_end:
    if(app_conf->normal_conf != NULL) free(app_conf->normal_conf);
    if(app_conf->scroll_conf != NULL) free(app_conf->scroll_conf);

    app_conf->normal_conf = NULL;
    app_conf->scroll_conf = NULL;

    return 0;
}


static
int apply_scroll_operator(struct app_setting *app_conf)
{
    int i, n_builtin_sop;

    /* number of builtin scroll operators */
    for(i = 0; builtin_scroll_op[i].name != NULL; i++) ;
    n_builtin_sop = i;

    /* allocate */
    app_conf->scroll_operator_num = n_builtin_sop;
    app_conf->scroll_operator_conf =
        (struct scroll_operator_conf *)
        malloc(sizeof(struct scroll_operator_conf) *
               app_conf->scroll_operator_num);
    if(app_conf->scroll_operator_conf == NULL) {
        return 0;
    }

    memset(app_conf->scroll_operator_conf, 0,
           sizeof(struct scroll_operator_conf) *
           app_conf->scroll_operator_num);

    /* setup builtin scroll operators */
    for(i = 0; i < n_builtin_sop; i++) {
        struct scroll_operator_conf *op_conf;

        op_conf = &app_conf->scroll_operator_conf[i];

        /* name of operator */
        op_conf->name = builtin_scroll_op[i].name;

        /* load operator */
        if(builtin_scroll_op[i].get_op_proc(&op_conf->proc,
                                            sizeof(scroll_op_procs_t),
                                            &ctx.sprocs) == 0) {
            memset(&op_conf->proc, 0, sizeof(scroll_op_procs_t));
            continue;
        }
    }

    /* for each operators */
    for(i = 0; i < app_conf->scroll_operator_num; i++) {
        struct scroll_operator_conf *op_conf;

        op_conf = &app_conf->scroll_operator_conf[i];

        /* check data */
        if(op_conf->proc.hdr.api_ver != MP_OP_API_VERSION ||
           op_conf->proc.hdr.type != MP_OP_TYPE_SCROLL) {
            memset(&op_conf->proc, 0, sizeof(scroll_op_procs_t));
        }

        /* fill configuration */
        op_conf->conf = get_conf_list(app_conf, S_EXP_NIL,
                                      L"operator-config",
                                      app_conf->scroll_operator_conf[i].name,
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
int apply_scroll_window(struct app_setting *app_conf)
{
    s_exp_data_t *list, *p;
    int i, j;

    /* number of scroll window configuration */
    list = get_conf_list(app_conf, S_EXP_NIL, L"scroll-window", NULL);

    /* allocate */
    app_conf->window_conf_num = s_exp_length(list);
    app_conf->window_conf =
        (struct scroll_window_conf *)
        malloc(sizeof(struct scroll_window_conf) * app_conf->window_conf_num);
    if(app_conf->window_conf == NULL) {
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
            log_printf(LOG_LEVEL_WARNING,
                       L"Invalid scroll-window format: ");
            log_print_s_exp(LOG_LEVEL_WARNING, e, 1);
            continue;
        }

        /* class/title regexp */
        if(get_class_title_regexp(S_EXP_CAR(e),
                                  &class_re, &title_re,
                                  &class_or_title) == 0) {
            log_printf(LOG_LEVEL_WARNING,
                       L"Invalid class/title regexp format: ");
            log_print_s_exp(LOG_LEVEL_WARNING, e, 1);
            continue;
        }

        /* operator name and args */
        if(S_EXP_CDR(e)->type != S_EXP_TYPE_CONS ||
           S_EXP_CADR(e)->type != S_EXP_TYPE_CONS ||
           S_EXP_CAADR(e)->type != S_EXP_TYPE_SYMBOL) {
            log_printf(LOG_LEVEL_WARNING, L"Invalid operator format: ");
            log_print_s_exp(LOG_LEVEL_WARNING, e, 1);
            continue;
        }

        op_name = S_EXP_CAADR(e)->symbol.name;
        op_arg = S_EXP_CADR(e);

        /* operator */
        op = NULL;
        for(j = 0; j < app_conf->scroll_operator_num; j++) {
            if(wcscmp(op_name,
                      app_conf->scroll_operator_conf[j].name) == 0) {
                op = &app_conf->scroll_operator_conf[j];
                break;
            }
        }
        if(op == NULL) {
            log_printf(LOG_LEVEL_WARNING, L"operator not found: ");
            log_print_s_exp(LOG_LEVEL_WARNING, S_EXP_CADR(e), 1);
            continue;
        }

        /* fill */
        app_conf->window_conf[i].class_regexp = class_re;
        app_conf->window_conf[i].title_regexp = title_re;
        app_conf->window_conf[i].class_or_title = class_or_title;
        app_conf->window_conf[i].op = op;
        app_conf->window_conf[i].args = op_arg;
        app_conf->window_conf[i].regexp = e;

        i += 1;
    }

    app_conf->window_conf_num = i;

    return 1;
}


static
int apply_setting(struct app_setting *app_conf)
{
    int ret;

    /* global setting */

    /* combination time */
    app_conf->comb_time = get_conf_int(app_conf, 300,
                                       L"global", L"combination-time",
                                       NULL);

    /* tray icon */
    {
        s_exp_data_t *t, *tt;

        t = get_conf_list(app_conf, S_EXP_NIL,
                          L"global", L"tray-icon",
                          NULL);
        tt = s_exp_nth(t, 0);
        if(tt == NULL || tt != S_EXP_FALSE) {
            app_conf->tray_icon_hide = 0;
            app_conf->tray_icon_file = get_nth_str(t, 0, NULL);
            app_conf->tray_icon_idx = get_nth_int(t, 1, 0);
        } else {
            app_conf->tray_icon_hide = 1;
            app_conf->tray_icon_file = NULL;
        }
    }

    /* process priority class */
    app_conf->priority_class = NORMAL_PRIORITY_CLASS;
    {
        s_exp_data_t *t;

        t = get_conf(app_conf, S_EXP_TYPE_SYMBOL,
                     L"global", L"priority", NULL);
        if(t != NULL) {
            int i;
            static struct uint_ptr_pair class_map[] = {
                /*{REALTIME_PRIORITY_CLASS, L"realtime"},*/
                {HIGH_PRIORITY_CLASS, L"high"},
                {ABOVE_NORMAL_PRIORITY_CLASS, L"above-normal"},
                {NORMAL_PRIORITY_CLASS, L"normal"},
                {BELOW_NORMAL_PRIORITY_CLASS, L"below-normal"},
                {IDLE_PRIORITY_CLASS, L"idle"},

                {0, NULL}
            };

            for(i = 0; class_map[i].ptr != NULL; i++) {
                if(wcscmp(t->symbol.name, class_map[i].ptr) == 0) {
                    app_conf->priority_class = class_map[i].key;
                    break;
                }
            }
        }
    }

    /* normal, scroll mode */
    ret = apply_mode_conf(app_conf);
    if(ret == 0) {
        return 0;
    }

    /* scroll operator */
    ret = apply_scroll_operator(app_conf);
    if(ret == 0) {
        return 0;
    }

    /* scroll window */
    ret = apply_scroll_window(app_conf);
    if(ret == 0) {
        return 0;
    }

    return 1;
}


static
void free_setting(struct app_setting *app_conf)
{
    free_s_exp(app_conf->conf_data);
    free(app_conf->normal_conf);
    free(app_conf->scroll_conf);
    free(app_conf->window_conf);
    free(app_conf->scroll_operator_conf);
}


int load_setting(LPWSTR conf_file, int force_apply)
{
    struct app_setting conf;

    memset(&conf, 0, sizeof(conf));

    conf.conf_file = conf_file;

    log_printf(LOG_LEVEL_NOTIFY, L"\n" L"Loading configuration...\n");

    /* load setting file */
    include_depth = 0;
    conf.conf_data = load_conf(conf.conf_file, S_EXP_NIL);
    if(conf.conf_data == NULL) {
        log_printf(LOG_LEVEL_WARNING, L"Config file not found\n");
        conf.conf_data = S_EXP_NIL;
    }

    if(S_EXP_ERROR(conf.conf_data)) {
        LPWSTR msg;

        msg = wcs_dup_from_u8s(conf.conf_data->error.descript, NULL);
        log_printf(LOG_LEVEL_ERROR, L"Failed to load: %ls\n", msg);
        free(msg);

        free_s_exp(conf.conf_data);

        if(! force_apply) {
            return 0;
        }

        conf.conf_data = S_EXP_NIL;
    }

    /* apply setting for config */
    if(apply_setting(&conf) == 0) {
        log_printf(LOG_LEVEL_ERROR, L"Failed to load config file\n");

        if(! force_apply) {
            free_setting(&conf);
            return 0;
        }
    }

    log_printf(LOG_LEVEL_NOTIFY, L"Load configuration completed\n");

    /* replace config */
    {
        struct app_setting prev_conf;
        struct mode_conf data;
        POINT pt;

        /* replace config data */
        memcpy(&prev_conf, &ctx.app_conf, sizeof(struct app_setting));
        memcpy(&ctx.app_conf, &conf, sizeof(struct app_setting));

        /* notify mode change */
        memset(&data, 0, sizeof(data));
        data.mode = MODE_CH_NORMAL;
        GetCursorPos(&pt);
        ctx.mode_data.cur_conf = ctx.app_conf.initial_conf;
        SendMessage(ctx.main_window, WM_MOUSEHOOK_MODECH,
                    MAKEWPARAM(pt.x, pt.y), (LPARAM)&data);

        /* notify tray icon change */
        SendMessage(ctx.main_window, WM_TASKTRAY_CH, 0, 0);

        /* set process priority */
        SetPriorityClass(GetCurrentProcess(), ctx.app_conf.priority_class);

        /* free previous config */
        free_setting(&prev_conf);
    }

    return 1;
}
