/*
 * scroll_op_scrollbar.h  -- 
 *
 */

#define FAKE_GSINFO_SHMEM_NAME \
L"Local\\mouse-processsor-{F9155CC0-3B30-447a-8194-26BA9CF1E322}"

struct fake_get_scrollbar_info_data {
    HHOOK hook[2];
    int hook_idx;
    UINT hook_subclass_msg;

    int valid;                  /* valid following data only if this is true */
    UINT msg;
    HWND hwnd;
    int bar;
    int org_pos;
    int track_pos;
};
typedef struct fake_get_scrollbar_info_data fake_gsinfo_data_t;
