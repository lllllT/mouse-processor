/*
 * operator.h  -- operator definition
 *
 * $Id: operator.h,v 1.4 2005/01/21 05:26:09 hos Exp $
 *
 */

#ifndef __OPERATOR_H__
#define __OPERATOR_H__ 1

#define _WIN32_WINNT 0x0500
#define UNICODE 1
#define _UNICODE 1

#include "s_exp.h"
#include <windows.h>


/* API version */
#define MP_OP_API_VERSION 0x0100

/* calling convention */
#define MP_OP_API __cdecl

/* type of operator */
enum {
    MP_OP_TYPE_SCROLL
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

typedef int (MP_OP_API * get_operator_proc_t)(scroll_op_procs_t *, int);


#endif /* __OPERATOR_H__ */
