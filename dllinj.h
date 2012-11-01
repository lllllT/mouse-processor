/*
 * dllinj.h  -- DLL injection
 *
 */

int replace_imported_proc(HMODULE mod,
                          void *target_proc, const char *target_proc_mod_name,
                          void *new_proc);
void *replace_all_imported_proc(const char *target_proc_mod_name,
                                const char *target_proc_name,
                                void *new_proc,
                                int to_restore);
