/*
 * main.h  --
 *
 * $Id: main.h,v 1.3 2004/12/28 03:44:44 hos Exp $
 *
 */


#define MOUSE_BTN_MAX 5

#define MOUSE_BTN_CONF_ENABLE_COMB 0x0001
#define MOUSE_BTN_CONF_REPLACE     0x0002

#define MOUSE_BTN_COMB_BTN        0x00010000
#define MOUSE_BTN_COMB_VALUE_MASK 0x0000ffff

struct mouse_conf {
    struct {
        unsigned int flags;

        unsigned int combination[MOUSE_BTN_MAX];
        int replace;
    } button[MOUSE_BTN_MAX];

    int comb_time;
};

struct app_context {
    HINSTANCE instance;
    HWND main_window;

    struct mouse_conf conf;
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
