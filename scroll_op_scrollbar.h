/*
 * scroll_op_scrollbar.h  -- 
 *
 * $Id: scroll_op_scrollbar.h,v 1.1 2005/02/01 11:28:25 hos Exp $
 *
 */

#define FAKE_GSINFO_SHMEM_NAME L"Local\\mouse-proces_fgsid"

struct fake_get_scrollbar_info_data {
    int valid;                  /* valid following data only if this is true */
    HWND hwnd;
    int bar;
    int track_pos;
};
typedef struct fake_get_scrollbar_info_data fake_gsinfo_data_t;
