/*
 * scroll_op.h  -- scroll operators
 *
 * $Id: operator.h,v 1.1 2005/01/12 09:39:47 hos Exp $
 *
 */

#ifndef __SCROLL_OP_H__
#define __SCROLL_OP_H__ 1

#include "s_exp.h"
#include <windows.h>


/* API version */
#define SCROLL_OP_API_VERSION 0x0100

/* calling convention */
#define SCROLL_OP_API __cdecl

/* arguments for scroll operator */
struct scroll_operator_arg {
    s_exp_data_t *conf;         /* operator-wide configuration */
    s_exp_data_t *arg;          /* inline argument */
    HWND hwnd;                  /* handle to target window */
    POINT pos;                  /* cursor position */
};
typedef struct scroll_operator_arg scroll_op_arg_t;

/* scroll operator procedure types */
typedef int (SCROLL_OP_API * get_context_size_proc_t)(const scroll_op_arg_t *);
typedef int (SCROLL_OP_API * init_context_proc_t)(void *, int,
                                                  const scroll_op_arg_t *);
typedef int (SCROLL_OP_API * scroll_proc_t)(void *, double, double);
typedef int (SCROLL_OP_API * end_scroll_proc_t)(void *);

/* scroll operator procedures */
struct scroll_operator_procs {
    get_context_size_proc_t get_context_size;
    init_context_proc_t init_context;
    scroll_proc_t scroll;
    end_scroll_proc_t end_scroll;
};
typedef struct scroll_operator_procs scroll_op_procs_t;

typedef int (SCROLL_OP_API * get_operator_proc_t)(scroll_op_procs_t *, int);


#endif /* __SCROLL_OP_H__ */
