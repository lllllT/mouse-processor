/*
 * shmem.c  -- shared memory
 *
 * $Id: shmem.c,v 1.1 2005/02/01 11:28:25 hos Exp $
 *
 */

#include <windows.h>


void *create_shared_mem(LPCWSTR name, DWORD size)
{
    HANDLE map;
    void *ptr;

    map = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                             0, size, name);
    if(map == NULL) {
        return NULL;
    }
    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(map);
        return NULL;
    }

    ptr = MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, size);

    CloseHandle(map);

    return ptr;
}

void *open_shared_mem(LPCWSTR name, DWORD size)
{
    HANDLE map;
    void *ptr;

    map = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
    if(map == NULL) {
        return NULL;
    }

    ptr = MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, size);

    CloseHandle(map);

    return ptr;
}

int close_shared_mem(void *ptr)
{
    return UnmapViewOfFile(ptr);
}