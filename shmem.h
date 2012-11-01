/*
 * shmem.h  -- shared memory
 *
 */

void *create_shared_mem(LPCWSTR name, DWORD size, HANDLE *hmap);
void *open_shared_mem(LPCWSTR name, DWORD size, HANDLE *hmap);
int close_shared_mem(void *ptr, HANDLE hmap);
