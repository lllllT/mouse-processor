/*
 * scroll_op_scrollbar.h  -- 
 *
 * $Id: scroll_op_scrollbar.h,v 1.3 2005/02/02 10:03:53 hos Exp $
 *
 */

#define FAKE_GSINFO_SHMEM_NAME \
L"Local\\mouse-processsor-{F9155CC0-3B30-447a-8194-26BA9CF1E322}"

struct fake_get_scrollbar_info_data {
    HHOOK hook[2];
    int hook_idx;
    UINT hook_subclass_msg;

    int valid;                  /* valid following data only if this is true */
    HWND hwnd;
    int bar;
    int track_pos;
};
typedef struct fake_get_scrollbar_info_data fake_gsinfo_data_t;
