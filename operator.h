/*
 * operator.h  -- operator definition
 *
 * $Id: operator.h,v 1.6 2005/02/03 09:51:13 hos Exp $
 *
 */

#ifndef __OPERATOR_H__
#define __OPERATOR_H__ 1

#include "s_exp.h"
#include <windows.h>


/* API version */
#define MP_OP_API_VERSION 0x0100

/* calling convention */
#define MP_OP_API __cdecl

/* type of operator */
enum {
    MP_OP_TYPE_SUPPORT,         /* support procedures */
    MP_OP_TYPE_SCROLL,          /* scroll operator */

    MP_OP_TYPE_END
};


/* arguments for operator */
struct operator_arg {
    const s_exp_data_t *conf;   /* operator-wide configuration */
    const s_exp_data_t *arg;    /* inline argument */
    HWND hwnd;                  /* handle to target window */
    POINT pos;                  /* cursor position */
};
typedef struct operator_arg op_arg_t;

/* operator procedure data header */
struct operator_procs_header {
    int api_ver;                /* target API version of operator */
    int type;                   /* type of operator */
};
typedef struct operator_procs_header op_procs_hdr_t;


/* support procedures provided by main module */
struct support_procs {
    op_procs_hdr_t hdr;

    /* logging procs */
    int (MP_OP_API * log_printf)(int level, const wchar_t *fmt, ...);
    int (MP_OP_API * log_s_exp)(int level, const s_exp_data_t *data,
                                int add_nl);
    int (MP_OP_API * log_lasterror)(int level, const wchar_t *msg, int add_nl);
    int (MP_OP_API * log_hresult)(int level, const wchar_t *msg, HRESULT hr,
                                  int add_nl);
};
typedef struct support_procs support_procs_t;

/* log level */
enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_NOTIFY,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
};


/* scroll operator procedure types */
typedef int (MP_OP_API * get_context_size_proc_t)(const op_arg_t *);
typedef int (MP_OP_API * init_context_proc_t)(void *, int, const op_arg_t *);
typedef int (MP_OP_API * scroll_proc_t)(void *, double, double);
typedef int (MP_OP_API * end_scroll_proc_t)(void *);

/* scroll operator procedures */
struct scroll_operator_procs {
    op_procs_hdr_t hdr;

    get_context_size_proc_t get_context_size;
    init_context_proc_t init_context;
    scroll_proc_t scroll;
    end_scroll_proc_t end_scroll;
};
typedef struct scroll_operator_procs scroll_op_procs_t;

typedef int (MP_OP_API * get_operator_proc_t)(scroll_op_procs_t *, int,
                                              const support_procs_t *);


#endif /* __OPERATOR_H__ */
