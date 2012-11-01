/*
 * scroll_op_utils.h  -- utility for scroll operators
 *
 */

struct scroll_op_mode_pair {
    wchar_t *name;
    int mode;
};

wchar_t *get_scroll_op_mode_name(const s_exp_data_t *arg,
                                 const s_exp_data_t *conf,
                                 const wchar_t *def_val);
int get_scroll_op_mode(struct scroll_op_mode_pair *mode_map,
                       const wchar_t *mode_name);

void get_scroll_op_xy_ratio(const s_exp_data_t *arg,
                            const s_exp_data_t *mode_conf,
                            double *x_ratio, double *y_ratio,
                            double def_x_ratio, double def_y_ratio);

int get_drag_scroll_delta(int length,
                          int min, int max, double page_ratio,
                          double delta, double *rest);
