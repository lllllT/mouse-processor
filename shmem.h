/*
 * shmem.h  -- shared memory
 *
 * $Id: shmem.h,v 1.1 2005/02/01 11:28:26 hos Exp $
 *
 */

void *create_shared_mem(LPCWSTR name, DWORD size);
void *open_shared_mem(LPCWSTR name, DWORD size);
int close_shared_mem(void *ptr);
