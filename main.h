/*
 * main.h  --
 *
 * $Id: main.h,v 1.12 2005/01/07 05:50:01 hos Exp $
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
#define MOUSE_ACT_NONE        0x00
#define MOUSE_ACT_BUTTON      0x01
#define MOUSE_ACT_WHEEL       0x02
#define MOUSE_ACT_WHEELPOST   0x03
#define MOUSE_ACT_MOVE        0x04
#define MOUSE_ACT_MODECH      0x10
#define MOUSE_ACT_SCROLL      0x20

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

/* mode change config */
struct mode_conf {
    int mode;

    union {
        struct {
            double x_ratio;
            double y_ratio;
        } scroll;
    };
};

/* custom action */
struct mouse_action {
    unsigned int code;

    union {
        int button;

        struct {
            double ratio;
            int tick;
        } wheel;

        struct {
            struct mouse_conf *mode;
            struct mode_conf data;
        } mode;
    } conf;

    union {
        double wheel;
    } data;
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

/* application context */
struct app_context {
    HINSTANCE instance;
    HWND main_window;

    LPWSTR conf_file;
    s_exp_data_t *conf_data;

    int comb_time;
    struct mouse_conf *conf;
    struct mouse_conf norm_conf;
    struct mouse_conf scroll_conf;

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

#define WM_MOUSEHOOK_SCROLL_MODE  (WM_APP + 10)
#define WM_MOUSEHOOK_SCROLLING    (WM_APP + 11)


int set_hook(void);
int clear_hook(void);


LRESULT scroll_mode(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT scrolling(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


HRESULT get_ie_target(HWND hwnd, int x, int y, IDispatch **elem);
HRESULT get_ie_elem_size(IDispatch *elem, SIZE *sz);
HRESULT init_ie_dpids(IDispatch *elem);

int scroll_ie_h(IDispatch *elem, int delta, int length);
int scroll_ie_v(IDispatch *elem, int delta, int length);

s_exp_data_t *load_conf(LPCWSTR conf_file);

s_exp_data_t *get_conf(int type, ...);
int get_conf_int(int def_val, ...);
double get_conf_double(int def_val, ...);
wchar_t *get_conf_string(const wchar_t *def_val, ...);
