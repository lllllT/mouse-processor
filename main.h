/*
 * main.h  --
 *
 * $Id: main.h,v 1.7 2004/12/31 21:33:00 hos Exp $
 *
 */


#define MOUSE_BTN_MAX 5

/* mouse_conf.button[].flags */
#define MOUSE_BTN_CONF_ENABLE_COMB 0x0001

/* mouse_action.code */
#define MOUSE_ACT_NONE   0x00
#define MOUSE_ACT_BUTTON 0x01
#define MOUSE_ACT_WHEEL  0x02
#define MOUSE_ACT_MOVE   0x03
#define MOUSE_ACT_MODECH 0x10
#define MOUSE_ACT_SCROLL 0x20

/* scroll_context.mode */
#define SCROLL_MODE_NATIVE_HV 0x00
#define SCROLL_MODE_NATIVE_V  0x01
#define SCROLL_MODE_NATIVE_H  0x02
#define SCROLL_MODE_LINESCRL  0x10
#define SCROLL_MODE_IE        0x20
#define SCROLL_MODE_WHEEL     0x30

/* app_context.ignore_btn_mask */
#define MOUSE_BTN_BIT(n) (1 << (n))

/* custom action */
struct mouse_action {
    unsigned int code;
    int data;
};

/* mouse configuration */
struct mouse_conf {
    int comb_time;

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

    int dx, dy;
};

/* application context */
struct app_context {
    HINSTANCE instance;
    HWND main_window;

    struct mouse_conf *conf;
    struct mouse_conf norm_conf;
    struct mouse_conf scroll_conf;

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

#define WM_MOUSEHOOK_SCROLL_BEGIN (WM_APP + 10)
#define WM_MOUSEHOOK_SCROLLING    (WM_APP + 11)
#define WM_MOUSEHOOK_SCROLL_END   (WM_APP + 12)


int set_hook(void);
int clear_hook(void);

LRESULT scroll_begin(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT scrolling(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT scroll_end(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
