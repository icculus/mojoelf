/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef _INCL_MACELF_H_
#define _INCL_MACELF_H_

#include "mojoelf.h"

// just some things shared between source files...

#ifndef MACELF_SUPPORT_NATIVE_OVERRIDE_SDL12
#define MACELF_SUPPORT_NATIVE_OVERRIDE_SDL12 1
#endif

#if 0
#define STUBBED(x) do {} while (0)
#else
#define STUBBED(x) do { \
    static int seen_this = 0; \
    if (!seen_this) { \
        seen_this = 1; \
        fprintf(stderr, "STUBBED: %s at %s (%s:%d)\n", x, __FUNCTION__, __FILE__, __LINE__); \
    } \
} while (0)
#endif

extern char *program_invocation_name;
extern const MOJOELF_Callbacks mojoelf_callbacks;

int build_trampolines(void);
int insert_symbol(const char *fn, void *ptr);
int remove_symbol(const char *fn);
void missing_symbol_called(const char *missing_symbol);
void warn_missing_native_symbol(const char *lib, const char *fn);

#if MACELF_SUPPORT_NATIVE_OVERRIDE_SDL12
void *load_native_sdl12(void);
void unload_native_sdl12(void *handle);
#endif

#endif

// end of macelf.h ...
