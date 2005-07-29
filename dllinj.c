/*
 * dllinj.c  -- DLL injection
 *
 * $Id: dllinj.c,v 1.7 2005/07/29 19:15:01 hos Exp $
 *
 */

#include <windows.h>
#include <psapi.h>


int replace_imported_proc(HMODULE mod,
                          void *target_proc, const char *target_proc_mod_name,
                          void *new_proc)
{
    char *base_addr;
    IMAGE_DOS_HEADER *dos_hdr;
    IMAGE_NT_HEADERS *nt_hdr;
    IMAGE_IMPORT_DESCRIPTOR *imp_desc;
    IMAGE_THUNK_DATA *thunk;

    base_addr = (char *)mod;

    dos_hdr = (IMAGE_DOS_HEADER *)(base_addr + 0);
    if(IsBadReadPtr(dos_hdr, sizeof(IMAGE_DOS_HEADER)) ||
       dos_hdr->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    nt_hdr = (IMAGE_NT_HEADERS *)(base_addr + dos_hdr->e_lfanew);
    if(IsBadReadPtr(nt_hdr, sizeof(IMAGE_NT_HEADERS)) ||
       nt_hdr->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    if(nt_hdr->OptionalHeader.
       DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0 ||
       nt_hdr->OptionalHeader.
       DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress == 0) {
        return 0;
    }

    imp_desc = (IMAGE_IMPORT_DESCRIPTOR *)
               (base_addr + nt_hdr->OptionalHeader.
                DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    for(; imp_desc->Name != 0; imp_desc++) {
        char *name;

        name = (char *)(base_addr + imp_desc->Name);
        if(lstrcmpiA(name, target_proc_mod_name) != 0) {
            continue;
        }

        for(thunk = (IMAGE_THUNK_DATA *)(base_addr + imp_desc->FirstThunk);
            thunk->u1.Function != 0; thunk++) {
            if((void *)thunk->u1.Function == target_proc) {
                BOOL ret, protect_changed = FALSE;
                DWORD org_protect = 0;

                if(IsBadWritePtr(&thunk->u1.Function,
                                 sizeof(thunk->u1.Function))) {
                    /* change protect for write access */
                    ret = VirtualProtect(&thunk->u1.Function,
                                         sizeof(thunk->u1.Function),
                                         PAGE_EXECUTE_READWRITE, &org_protect);
                    if(ret == 0) {
                        continue;
                    }

                    protect_changed = TRUE;
                }

                /* replace pointer to function */
                *((void **)&thunk->u1.Function) = new_proc;

                if(protect_changed) {
                    DWORD prt;

                    /* restore protect */
                    ret = VirtualProtect(&thunk->u1.Function,
                                         sizeof(thunk->u1.Function),
                                         org_protect, &prt);
                }
            }
        }
    }

    return 1;
}

void *replace_all_imported_proc(const char *target_proc_mod_name,
                                const char *target_proc_name,
                                void *new_proc,
                                int to_restore)
{
    HMODULE target_proc_mod;
    void *target_proc;
    void *from, *to;

    target_proc_mod = GetModuleHandleA(target_proc_mod_name);
    if(target_proc_mod == NULL) {
        return NULL;
    }

    target_proc = GetProcAddress(target_proc_mod, target_proc_name);
    if(target_proc == NULL) {
        return NULL;
    }

    if(! to_restore) {
        from = target_proc;
        to = new_proc;
    } else {
        from = new_proc;
        to = target_proc;
    }

    {
        HMODULE mods[256];
        DWORD size;
        int n, i;

        if(EnumProcessModules(GetCurrentProcess(),
                              mods, sizeof(mods), &size) == 0) {
            return NULL;
        }

        n = size / sizeof(HMODULE);
        n = (n > 256 ? 256 : n);

        /* for each loaded modules */
        for(i = 0; i < n; i++) {
            replace_imported_proc(mods[i], from, target_proc_mod_name, to);
        }
    }

    return target_proc;
}
