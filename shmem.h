/*
 * shmem.h  -- shared memory
 *
 * $Id: shmem.h,v 1.2 2005/02/01 17:03:50 hos Exp $
 *
 */

void *create_shared_mem(LPCWSTR name, DWORD size, HANDLE *hmap);
void *open_shared_mem(LPCWSTR name, DWORD size, HANDLE *hmap);
int close_shared_mem(void *ptr, HANDLE hmap);
