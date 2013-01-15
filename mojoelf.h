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

typedef void *(*MOJOELF_SymbolCallback)(const char *sym);
void *MOJOELF_dlopen_mem(const void *buf, const long buflen,
                         MOJOELF_SymbolCallback resolver);
void *MOJOELF_dlopen_file(const char *fname, MOJOELF_SymbolCallback resolver);
void *MOJOELF_dlsym(void *lib, const char *sym);
void MOJOELF_dlclose(void *lib);
const char *MOJOELF_dlerror(void);
const void *MOJOELF_getentry(void *lib);

#ifdef __cplusplus
}
#endif

#endif

/* end of mojoelf.h ... */


