/*
 * main.h  --
 *
 * $Id: main.h,v 1.2 2004/12/27 09:22:14 hos Exp $
 *
 */


#define MOUSE_BTN_MAX 5

#define MOUSE_BTN_CONF_ENABLE_COMB 0x0001
#define MOUSE_BTN_CONF_REPLACE     0x0002

struct mouse_conf {
    struct {
        unsigned int flags;

        unsigned char combination[MOUSE_BTN_MAX];
        int replace;
    } button[MOUSE_BTN_MAX];
};

struct app_context {
    HINSTANCE instance;
    HWND main_window;

    struct mouse_conf conf;
    int pressed;
    MSLLHOOKSTRUCT pressed_btn;
};

extern struct app_context ctx;


#define MOTION_DOWN  0
#define MOTION_UP    1
#define MOTION_MOVE  2
#define MOTION_WHEEL 3

#define WM_MOUSEHOOK_MOTION (WM_APP + 10)


int set_hook(void);
int clear_hook(void);
