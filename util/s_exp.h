/*
 * s_exp.h  -- s-expression
 *
 */

#ifndef __S_EXP_H__
#define __S_EXP_H__ 1


#include <wchar.h>
#include <stdarg.h>


typedef enum {
    S_EXP_TYPE_TRUE,
    S_EXP_TYPE_FALSE,
    S_EXP_TYPE_NIL,

    S_EXP_TYPE_CONS,
    S_EXP_TYPE_INTEGER,
    S_EXP_TYPE_NUMBER = S_EXP_TYPE_INTEGER,
    S_EXP_TYPE_FLONUM,
    S_EXP_TYPE_STRING,
    S_EXP_TYPE_SYMBOL,

    S_EXP_TYPE_ERROR
} s_exp_type_t;

typedef struct s_exp_data {
    s_exp_type_t type;

    unsigned int ref_cnt;

    union {
        struct {
            struct s_exp_data *car;
            struct s_exp_data *cdr;
        } cons;

        struct {
            int val;
        } number;

        struct {
            double val;
        } flonum;

        struct {
            wchar_t *str;
            int len;
            int alloc_len;
        } string;

        struct {
            wchar_t *name;
        } symbol;

        struct {
            char *descript;
        } error;
    };
} s_exp_data_t;

typedef struct s_exp_read_context s_exp_read_context_t;

extern s_exp_data_t s_exp_true, s_exp_false, s_exp_nil;

#define S_EXP_TRUE (&s_exp_true)
#define S_EXP_FALSE (&s_exp_false)
#define S_EXP_NIL (&s_exp_nil)

#define S_EXP_ERROR(sexp) ((sexp)->type == S_EXP_TYPE_ERROR)

#define S_EXP_CAR(cell) ((cell)->cons.car)
#define S_EXP_CDR(cell) ((cell)->cons.cdr)

#define S_EXP_CAAR(cell) S_EXP_CAR(S_EXP_CAR(cell))
#define S_EXP_CADR(cell) S_EXP_CAR(S_EXP_CDR(cell))
#define S_EXP_CDAR(cell) S_EXP_CDR(S_EXP_CAR(cell))
#define S_EXP_CDDR(cell) S_EXP_CDR(S_EXP_CDR(cell))

#define S_EXP_CAAAR(cell) S_EXP_CAR(S_EXP_CAAR(cell))
#define S_EXP_CAADR(cell) S_EXP_CAR(S_EXP_CADR(cell))
#define S_EXP_CADAR(cell) S_EXP_CAR(S_EXP_CDAR(cell))
#define S_EXP_CADDR(cell) S_EXP_CAR(S_EXP_CDDR(cell))
#define S_EXP_CDAAR(cell) S_EXP_CDR(S_EXP_CAAR(cell))
#define S_EXP_CDADR(cell) S_EXP_CDR(S_EXP_CADR(cell))
#define S_EXP_CDDAR(cell) S_EXP_CDR(S_EXP_CDAR(cell))
#define S_EXP_CDDDR(cell) S_EXP_CDR(S_EXP_CDDR(cell))

#define S_EXP_CAAAAR(cell) S_EXP_CAR(S_EXP_CAAAR(cell))
#define S_EXP_CAAADR(cell) S_EXP_CAR(S_EXP_CAADR(cell))
#define S_EXP_CAADAR(cell) S_EXP_CAR(S_EXP_CADAR(cell))
#define S_EXP_CAADDR(cell) S_EXP_CAR(S_EXP_CADDR(cell))
#define S_EXP_CADAAR(cell) S_EXP_CAR(S_EXP_CDAAR(cell))
#define S_EXP_CADADR(cell) S_EXP_CAR(S_EXP_CDADR(cell))
#define S_EXP_CADDAR(cell) S_EXP_CAR(S_EXP_CDDAR(cell))
#define S_EXP_CADDDR(cell) S_EXP_CAR(S_EXP_CDDDR(cell))
#define S_EXP_CDAAAR(cell) S_EXP_CDR(S_EXP_CAAAR(cell))
#define S_EXP_CDAADR(cell) S_EXP_CDR(S_EXP_CAADR(cell))
#define S_EXP_CDADAR(cell) S_EXP_CDR(S_EXP_CADAR(cell))
#define S_EXP_CDADDR(cell) S_EXP_CDR(S_EXP_CADDR(cell))
#define S_EXP_CDDAAR(cell) S_EXP_CDR(S_EXP_CDAAR(cell))
#define S_EXP_CDDADR(cell) S_EXP_CDR(S_EXP_CDADR(cell))
#define S_EXP_CDDDAR(cell) S_EXP_CDR(S_EXP_CDDAR(cell))
#define S_EXP_CDDDDR(cell) S_EXP_CDR(S_EXP_CDDDR(cell))

#define S_EXP_FOR_EACH(list, p) \
  for((p) = (list); (p)->type == S_EXP_TYPE_CONS; (p) = S_EXP_CDR(p))


s_exp_read_context_t *open_s_exp_read_context(const char *file);
s_exp_read_context_t *open_s_exp_read_context_f(FILE *fp, const char *name);
int close_s_exp_read_context(s_exp_read_context_t *ctx);

s_exp_data_t *read_s_exp(s_exp_read_context_t *ctx);
s_exp_data_t *read_all_s_exp(s_exp_read_context_t *ctx);
int write_s_exp(FILE *fp, const s_exp_data_t *data);
unsigned char *u8s_write_s_exp(const s_exp_data_t *data);

s_exp_data_t *s_exp_cons(s_exp_data_t *car, s_exp_data_t *cdr);
s_exp_data_t *s_exp_intern(const wchar_t *str);

s_exp_data_t *s_exp_assq_get(const s_exp_data_t *alist, const wchar_t *sym);
s_exp_data_t *s_exp_assq(const s_exp_data_t *alist, const wchar_t *sym);
s_exp_data_t *s_exp_massq(const s_exp_data_t *alist, int type, ...);
s_exp_data_t *s_exp_massq_v(const s_exp_data_t *alist, int type, va_list ap);
s_exp_data_t *s_exp_nth(const s_exp_data_t *list, int nth);
s_exp_data_t *s_exp_nth_cdr(const s_exp_data_t *list, int nth);
int s_exp_length(const s_exp_data_t *list);

void free_s_exp(s_exp_data_t *data);


#endif /* __S_EXP_H__ */
