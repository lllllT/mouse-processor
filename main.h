/*
 * main.h  --
 *
 * $Id: main.h,v 1.4 2004/12/30 14:29:46 hos Exp $
 *
 */


#define MOUSE_BTN_MAX 5

/* mouse_conf.button[].flags */
#define MOUSE_BTN_CONF_ENABLE_COMB 0x0001
#define MOUSE_BTN_CONF_ACTION      0x0002

/* mouse_action.code */
#define MOUSE_ACT_NONE   0x0000
#define MOUSE_ACT_BUTTON 0x0001

/* app_context.ignore_btn_mask */
#define MOUSE_BTN_BIT(n) (1 << (n))

/* custom action */
struct mouse_action {
    unsigned int code;
    int data;
};

/* mouse configuration */
struct mouse_conf {
    struct {
        unsigned int flags;

        struct mouse_action act;
        struct mouse_action comb_act[MOUSE_BTN_MAX];
    } button[MOUSE_BTN_MAX];

    int comb_time;
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
};

extern struct app_context ctx;


#define MOTION_DOWN  0
#define MOTION_UP    1
#define MOTION_MOVE  2
#define MOTION_WHEEL 3

#define WM_MOUSEHOOK_MOTION (WM_APP + 10)


int set_hook(void);
int clear_hook(void);
