/*
 * scroll_op_scrollbar.h  -- 
 *
 * $Id: scroll_op_scrollbar.h,v 1.2 2005/02/01 17:03:50 hos Exp $
 *
 */

#define FAKE_GSINFO_SHMEM_NAME \
L"Local\\mouse-processsor-{F9155CC0-3B30-447a-8194-26BA9CF1E322}"

struct fake_get_scrollbar_info_data {
    int valid;                  /* valid following data only if this is true */
    HWND hwnd;
    int bar;
    int track_pos;
};
typedef struct fake_get_scrollbar_info_data fake_gsinfo_data_t;
