/*
 * main.h  --
 *
 * $Id: main.h,v 1.15 2005/01/09 13:56:59 hos Exp $
 *
 */

#include <windows.h>
#include <tchar.h>
#include <oleauto.h>
#include "s_exp.h"


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

/* scroll context */
struct scroll_context {
    POINT start_pt;
    int mode;

    HWND target;
    SIZE target_size;

    IDispatch *ie_target;

    double x_ratio, y_ratio;
    double dx, dy;

    double dw;
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
};

/* application context */
struct app_context {
    HINSTANCE instance;
    HWND main_window;

    struct app_setting app_conf;

    struct {
        double x_ratio;
        double y_ratio;
        int tick;
    } scroll_wheel;
    struct {
        double x_ratio;
        double y_ratio;
    } scroll_line;

    unsigned int ignore_btn_mask;

    int pressed;
    int pressed_btn;
    MSLLHOOKSTRUCT pressed_btn_data;

    int combinated;
    int combination[MOUSE_BTN_MAX * 3];

    struct scroll_context scroll_data;
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


HRESULT get_ie_target(HWND hwnd, int x, int y, IDispatch **elem);
HRESULT get_ie_elem_size(IDispatch *elem, SIZE *sz);
HRESULT init_ie_dpids(IDispatch *elem);

int scroll_ie_h(IDispatch *elem, int delta, int length);
int scroll_ie_v(IDispatch *elem, int delta, int length);

s_exp_data_t *load_conf(LPCWSTR conf_file);
int apply_setting(void);
