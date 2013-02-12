/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// Native overrides for X11...

// You absolutely must build this file with -mstackrealign.
//  A lot of these functions are here only as a trampoline: the ELF code calls
//  it, it aligns the stack to 16 bytes thanks to -mstackrealign, and then
//  calls the actual function in the system library.

#include "macelf.h"

#if MACELF_SUPPORT_NATIVE_OVERRIDE_X11

#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/extensions/extutil.h>
#include <X11/extensions/Xshm.h>

typedef int (*XSynchronizeRetType)(Display*,Bool);
typedef int (*XSetAfterFunctionRetType)(Display*,int (*)(Display*));
typedef Bool (*XESetWireToEventRetType)(Display*,XEvent*,xEvent*);
typedef Status (*XESetEventToWireRetType)(Display*,XEvent*,xEvent*);

#define MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    typedef typ (*nativefntype_x11_##fn) params; \
    static nativefntype_x11_##fn pnativefn_x11_##fn = NULL;
#define MACTRAMPOLINE(typ,fn,params,args,ret) \
    MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    static typ mactrampoline_x11_##fn params { \
        ret pnativefn_x11_##fn args; \
    }
#include "mactrampolines_x11.h"
#include "mactrampolines_xext.h"
#undef MACTRAMPOLINE_OVERRIDE
#undef MACTRAMPOLINE

void *load_native_x11(void)
{
    const char *libname = "/usr/X11/lib/libX11.6.dylib";
    void *handle = dlopen(libname, RTLD_NOW);
    if (handle == NULL)
    {
        fprintf(stderr, "WARNING: tried to load native libX11 and failed: %s\n", dlerror());
        fprintf(stderr, "WARNING:  You might need to install Xquartz from http://xquartz.macosforge.org/\n");
    } // if
    else
    {
        // if a symbol is missing, we don't care. We cast a wide net, and many are expected to not exist on Mac OS X.
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            pnativefn_x11_##fn = (nativefntype_x11_##fn) dlsym(handle, #fn); \
            if (pnativefn_x11_##fn != NULL) { \
                insert_symbol(#fn, mactrampoline_x11_##fn); \
            } \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_x11.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE
    } // else

    return handle;
} // load_native_x11

void unload_native_x11(void *handle)
{
    if (handle != NULL)
    {
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            remove_symbol(#fn); \
            pnativefn_x11_##fn = NULL; \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_x11.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE

        dlclose(handle);
    } // if
} // unload_native_x11

void *load_native_xext(void)
{
    const char *libname = "/usr/X11/lib/libXext.6.dylib";
    void *handle = dlopen(libname, RTLD_NOW);
    if (handle == NULL)
    {
        fprintf(stderr, "WARNING: tried to load native libXext and failed: %s\n", dlerror());
        fprintf(stderr, "WARNING:  You might need to install Xquartz from http://xquartz.macosforge.org/\n");
    } // if
    else
    {
        // if a symbol is missing, we don't care. We cast a wide net, and many are expected to not exist on Mac OS X.
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            pnativefn_x11_##fn = (nativefntype_x11_##fn) dlsym(handle, #fn); \
            if (pnativefn_x11_##fn != NULL) { \
                insert_symbol(#fn, mactrampoline_x11_##fn); \
            } \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_xext.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE
    } // else

    return handle;
} // load_native_xext

void unload_native_xext(void *handle)
{
    if (handle != NULL)
    {
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            remove_symbol(#fn); \
            pnativefn_x11_##fn = NULL; \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_xext.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE

        dlclose(handle);
    } // if
} // unload_native_xext

#endif

// end of mactrampolines_x11.c ...
