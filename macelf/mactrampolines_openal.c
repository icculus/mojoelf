/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// Native overrides for OpenAL...

// You absolutely must build this file with -mstackrealign.
//  A lot of these functions are here only as a trampoline: the ELF code calls
//  it, it aligns the stack to 16 bytes thanks to -mstackrealign, and then
//  calls the actual function in the system library.

#include "macelf.h"

#if MACELF_SUPPORT_NATIVE_OVERRIDE_OPENAL

#include <stdio.h>
#include <dlfcn.h>
#include <assert.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/MacOSX_OALExtensions.h>


#define MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    typedef typ (*nativefntype_openal_##fn) params; \
    static nativefntype_openal_##fn pnativefn_openal_##fn = NULL;
#define MACTRAMPOLINE(typ,fn,params,args,ret) \
    MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    static typ mactrampoline_openal_##fn params { \
        ret pnativefn_openal_##fn args; \
    }
#include "mactrampolines_openal.h"
#undef MACTRAMPOLINE_OVERRIDE
#undef MACTRAMPOLINE


// override a few things...

static void *mactrampoline_openal_alGetProcAddress(const ALchar *sym)
{
    return find_symbol((const char *) sym);  // we only want symbols we can trampoline.
} // mactrampoline_openal_alGetProcAddress

static void *mactrampoline_openal_alcGetProcAddress(ALCdevice *dev, const ALCchar *sym)
{
    return find_symbol((const char *) sym);  // we only want symbols we can trampoline.
} // mactrampoline_openal_alcGetProcAddress


void *load_native_openal(void)
{
    const char *libname = "/System/Library/Frameworks/OpenAL.framework/OpenAL";
    void *handle = dlopen(libname, RTLD_NOW);
    if (handle == NULL)
        fprintf(stderr, "WARNING: tried to load native OpenAL and failed: %s\n", dlerror());
    else
    {
        // if a symbol is missing, we don't care. We cast a wide net, and many are expected to not exist on Mac OS X.
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            pnativefn_openal_##fn = (nativefntype_openal_##fn) dlsym(handle, #fn); \
            if (pnativefn_openal_##fn != NULL) { \
                insert_symbol(#fn, mactrampoline_openal_##fn); \
            } \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_openal.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE
    } // else

    return handle;
} // load_native_openal

void unload_native_openal(void *handle)
{
    if (handle != NULL)
    {
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            remove_symbol(#fn); \
            pnativefn_openal_##fn = NULL; \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_openal.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE

        dlclose(handle);
    } // if
} // unload_native_openal

#endif

// end of mactrampolines_openal.c ...

