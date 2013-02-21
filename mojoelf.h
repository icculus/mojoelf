/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef INCL_MOJOELF_H
#define INCL_MOJOELF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*MOJOELF_LoaderCallback)(const char *soname, const char *rpath, const char *runpath);
typedef void *(*MOJOELF_ResolverCallback)(void *handle, const char *sym);
typedef void (*MOJOELF_UnloaderCallback)(void *handle);

typedef struct MOJOELF_Callbacks
{
    MOJOELF_LoaderCallback loader;
    MOJOELF_ResolverCallback resolver;
    MOJOELF_UnloaderCallback unloader;
} MOJOELF_Callbacks;

void *MOJOELF_dlopen_mem(const void *buf, const long buflen, const MOJOELF_Callbacks *cb);
void *MOJOELF_dlopen_file(const char *fname, const MOJOELF_Callbacks *cb);
void *MOJOELF_dlsym(void *lib, const char *sym);
void MOJOELF_dlclose(void *lib);
const char *MOJOELF_dlerror(void);
const void *MOJOELF_getentry(void *lib);
void MOJOELF_getmmaprange(void *lib, void **addr, unsigned long *len);

#ifdef __cplusplus
}
#endif

#endif

/* end of mojoelf.h ... */


