/*
 * conf.h  -- configuration
 *
 * $Id: conf.c,v 1.3 2005/01/07 04:52:24 hos Exp $
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
    return s_exp_massq_v(ctx.conf_data, type, ap);
}

s_exp_data_t *get_conf(int type, ...)
{
    va_list ap;
    s_exp_data_t *data;

    va_start(ap, type);
    data = get_conf_v(type, ap);
    va_end(ap);

    return data;
}

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

double get_conf_double(int def_val, ...)
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
            return (double)data->number.val;
        } else {
            return data->flonum.val;
        }
    }
}

wchar_t *get_conf_string(const wchar_t *def_val, ...)
{
    va_list ap;
    s_exp_data_t *data;

    va_start(ap, def_val);
    data = get_conf_v(S_EXP_TYPE_STRING, ap);
    va_end(ap);

    if(data == NULL) {
        return (wchar_t *)def_val;
    } else {
        return data->string.str;
    }
}
