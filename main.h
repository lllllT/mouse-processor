/*
 * main.h  --
 *
 * $Id: main.h,v 1.38 2005/02/03 09:51:12 hos Exp $
 *
 */

#include <windows.h>
#include <tchar.h>
#include <oleauto.h>
#include "s_exp.h"
#include "operator.h"


#define MOUSE_BTN_MAX 5

/* mouse_conf.button[].flags */
#define MOUSE_BTN_CONF_ENABLE_COMB 0x0001

/* mouse_action.code */
#define MOUSE_ACT_NOTHING     0x00 /* for all */
#define MOUSE_ACT_BUTTON_D    0x01 /* for button */
#define MOUSE_ACT_BUTTON_U    0x02 /* for button */
#define MOUSE_ACT_WHEEL       0x03 /* for wheel */
#define MOUSE_ACT_WHEELPOST   0x04 /* for wheel */
#define MOUSE_ACT_MOVE        0x05 /* for move */
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

        struct mouse_action d_act;
        struct mouse_action u_act;
        struct mouse_action comb_d_act[MOUSE_BTN_MAX];
        struct mouse_action comb_u_act[MOUSE_BTN_MAX];
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

    int tray_icon_hide;
    LPWSTR tray_icon_file;
    int tray_icon_idx;

    DWORD priority_class;

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
    struct mouse_conf *cur_conf;
    int cur_mode;

    POINT start_pt;

    union {
        struct scroll_mode_context scroll;
    };
};

/* application context */
struct app_context {
    HINSTANCE instance;
    HWND main_window;
    HWND log_window;
    HICON tray_icon;

    struct app_setting app_conf;

    struct hook_context hook_data;

    struct mode_context mode_data;

    support_procs_t sprocs;
};

extern struct app_context ctx;


#define WM_TASKTRAY           (WM_APP + 1)
#define WM_TASKTRAY_CH        (WM_APP + 2)
#define WM_MOUSEHOOK_MODECH   (WM_APP + 10)
#define WM_MOUSEHOOK_MODEMSG  (WM_APP + 11)

#define MOTION_DOWN  0
#define MOTION_UP    1
#define MOTION_MOVE  2
#define MOTION_WHEEL 3


int set_hook(void);
int clear_hook(void);


LRESULT scroll_modech(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT scroll_modemsg(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


int load_setting(LPWSTR conf_file, int force_apply);


int MP_OP_API window_scrollbar_get_operator(scroll_op_procs_t *op, int size,
                                            const support_procs_t *sprocs);
int MP_OP_API neighborhood_scrollbar_get_operator(scroll_op_procs_t *op,
                                                  int size,
                                                  const support_procs_t *spr);
int MP_OP_API scrollbar_control_get_operator(scroll_op_procs_t *op, int size,
                                             const support_procs_t *sprocs);
int MP_OP_API trackbar_control_get_operator(scroll_op_procs_t *op, int size,
                                            const support_procs_t *sprocs);
int MP_OP_API ie_scroll_get_operator(scroll_op_procs_t *op, int size,
                                     const support_procs_t *sprocs);
int MP_OP_API wheel_message_get_operator(scroll_op_procs_t *op, int size,
                                         const support_procs_t *sprocs);

struct scroll_operator_def {
    wchar_t *name;
    get_operator_proc_t get_op_proc;
};
extern struct scroll_operator_def builtin_scroll_op[];


int create_logger(void);
int destroy_logger(void);
int show_logger(BOOL show);

int MP_OP_API log_printf(int level, const wchar_t *fmt, ...);
int MP_OP_API log_print_s_exp(int level, const s_exp_data_t *data, int add_nl);
int MP_OP_API log_print_lasterror(int level, const wchar_t *msg, int add_nl);
int MP_OP_API log_print_hresult(int level, const wchar_t *msg, HRESULT hr,
                                int add_nl);


void error_message_le(const char *msg);
void error_message_hr(const char *msg, HRESULT hr);


HWND get_window_for_mouse_input(POINT spt);
void get_hierarchial_window_class_title(HWND hwnd,
                                        LPWSTR *class, LPWSTR *title);


HRESULT init_regexp(void);
int is_regexp_match(BSTR re_str, BSTR test_str);
