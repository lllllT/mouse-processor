/*
 * main.h  --
 *
 * $Id: main.h,v 1.26 2005/01/15 15:02:13 hos Exp $
 *
 */

#define _WIN32_WINNT 0x0500
#define UNICODE 1
#define _UNICODE 1


#include <windows.h>
#include <tchar.h>
#include <oleauto.h>
#include "s_exp.h"
#include "scroll_op.h"


#define MOUSE_BTN_MAX 5

/* mouse_conf.button[].flags */
#define MOUSE_BTN_CONF_ENABLE_COMB 0x0001

/* mouse_action.code */
#define MOUSE_ACT_NONE        0x00 /* for all */
#define MOUSE_ACT_BUTTON      0x01 /* for button */
#define MOUSE_ACT_WHEEL       0x02 /* for wheel */
#define MOUSE_ACT_WHEELPOST   0x03 /* for wheel */
#define MOUSE_ACT_MOVE        0x04 /* for move */
#define MOUSE_ACT_MODECH      0x10 /* for button */
#define MOUSE_ACT_MODEMSG     0x11 /* for button, move */

/* mode_conf.mode */
#define MODE_CH_NORMAL        0x00
#define MODE_CH_SCROLL        0x01
#define MODE_MSG_SCROLL       0x10
#define MODE_MSG_MUL_RATIO    0x20
#define MODE_MSG_SET_RATIO    0x21

/* scroll_context.mode */
#define SCROLL_MODE_NATIVE_HV 0x00
#define SCROLL_MODE_NATIVE_V  0x01
#define SCROLL_MODE_NATIVE_H  0x02
#define SCROLL_MODE_LINESCRL  0x10
#define SCROLL_MODE_IE        0x20
#define SCROLL_MODE_WHEEL     0x30

/* app_context.ignore_btn_mask */
#define MOUSE_BTN_BIT(n) (1 << (n))

struct mouse_conf;

/* mode config */
struct mode_conf {
    int mode;

    union {
        struct {
            double x;
            double y;
        } ratio;
    };
};

/* custom action */
struct mouse_action {
    unsigned int code;

    union {
        int button;

        struct {
            struct mouse_conf *mode;
            struct mode_conf data;
        } mode_change;

        struct {
            struct mode_conf data;
        } mode_msg;
    } conf;
};

/* mouse configuration */
struct mouse_conf {
    struct {
        unsigned int flags;

        struct mouse_action act;
        struct mouse_action comb_act[MOUSE_BTN_MAX];
    } button[MOUSE_BTN_MAX];

    struct mouse_action move_act;
    struct mouse_action wheel_act;

    wchar_t *mode_name;

    union {
        struct {
            double x_ratio;
            double y_ratio;
        } scroll_mode;
    };
};

/* scroll operator configuration */
struct scroll_operator_conf {
    LPWSTR name;

    struct scroll_operator_procs proc;
    s_exp_data_t *conf;
};

/* window configuration */
struct scroll_window_conf {
    LPWSTR class_regexp;
    LPWSTR title_regexp;
    int class_or_title;
    s_exp_data_t *regexp;

    struct scroll_operator_conf *op;
    s_exp_data_t *args;
};

/* application setting */
struct app_setting {
    LPWSTR conf_file;
    s_exp_data_t *conf_data;

    int comb_time;

    struct mouse_conf *cur_conf;

    int normal_conf_num;
    struct mouse_conf *normal_conf;

    int scroll_conf_num;
    struct mouse_conf *scroll_conf;

    int window_conf_num;
    struct scroll_window_conf *window_conf;

    int scroll_operator_num;
    struct scroll_operator_conf *scroll_operator_conf;
};

/* hook context */
struct hook_context {
    unsigned int ignore_btn_mask;

    int pressed;
    int pressed_btn;
    MSLLHOOKSTRUCT pressed_btn_data;

    int combinated;
    int combination[MOUSE_BTN_MAX * 3];
};

/* scroll mode context */
struct scroll_mode_context {
    POINT start_pt;

    HWND target;
    LPWSTR class;
    LPWSTR title;

    double x_ratio, y_ratio;
    double dx, dy;

    struct scroll_operator_conf *op;
    int op_context_size;
    void *op_context;
};

/* mode context */
struct mode_context {
    int cur_mode;

    union {
        struct scroll_mode_context scroll;
    };
};

/* application context */
struct app_context {
    HINSTANCE instance;
    HWND main_window;
    HWND log_window;

    struct app_setting app_conf;

    struct hook_context hook_data;

    struct mode_context mode_data;
};

extern struct app_context ctx;


#define MOTION_DOWN  0
#define MOTION_UP    1
#define MOTION_MOVE  2
#define MOTION_WHEEL 3

#define WM_MOUSEHOOK_MODECH   (WM_APP + 10)
#define WM_MOUSEHOOK_MODEMSG  (WM_APP + 11)


int set_hook(void);
int clear_hook(void);


LRESULT scroll_modech(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT scroll_modemsg(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


s_exp_data_t *load_conf(LPCWSTR conf_file);
int apply_setting(void);


int SCROLL_OP_API window_scrollbar_get_operator(scroll_op_procs_t *op,
                                                int api_ver);
int SCROLL_OP_API neighborhood_scrollbar_get_operator(scroll_op_procs_t *op,
                                                      int api_ver);
int SCROLL_OP_API scrollbar_control_get_operator(scroll_op_procs_t *op,
                                                 int api_ver);
int SCROLL_OP_API trackbar_control_get_operator(scroll_op_procs_t *op,
                                                int api_ver);
int SCROLL_OP_API ie_scroll_get_operator(scroll_op_procs_t *op,
                                         int api_ver);
int SCROLL_OP_API wheel_message_get_operator(scroll_op_procs_t *op,
                                             int api_ver);

struct scroll_operator_def {
    wchar_t *name;
    get_operator_proc_t get_op_proc;
};
extern struct scroll_operator_def builtin_scroll_op[];


enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_NOTIFY,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
};

int create_logger(void);
int destroy_logger(void);
int show_logger(BOOL show);

int log_printf(int level, const wchar_t *fmt, ...);
int log_print_s_exp(int level, const s_exp_data_t *data);


int is_regexp_match(BSTR re_str, BSTR test_str);
