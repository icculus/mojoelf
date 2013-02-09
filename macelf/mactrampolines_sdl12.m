/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// Native overrides for SDL 1.2...

// You absolutely must build this file with -mstackrealign.
//  A lot of these functions are here only as a trampoline: the ELF code calls
//  it, it aligns the stack to 16 bytes thanks to -mstackrealign, and then
//  calls the actual function in the system library.

#include "macelf.h"

#if MACELF_SUPPORT_NATIVE_OVERRIDE_SDL12

#include <stdio.h>
#include <stdarg.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <dlfcn.h>
#include "SDL.h"


#define MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    typedef typ (*nativefntype_sdl12_##fn) params; \
    static nativefntype_sdl12_##fn pnativefn_sdl12_##fn = NULL;
#define MACTRAMPOLINE(typ,fn,params,args,ret) \
    MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    static typ mactrampoline_sdl12_##fn params { \
        ret pnativefn_sdl12_##fn args; \
    }
#include "mactrampolines_sdl12.h"
#undef MACTRAMPOLINE_OVERRIDE
#undef MACTRAMPOLINE


// override a few things...

// SDL_Init needs a little startup code, since we don't use SDLmain.
static int mactrampoline_sdl12_SDL_Init(Uint32 flags)
{
    static int first_call = 1;
    if (first_call)
    {
        first_call = 0;

        ProcessSerialNumber psn;
        if (!GetCurrentProcess(&psn))
        {
            TransformProcessType(&psn, kProcessTransformToForegroundApplication);
            SetFrontProcess(&psn);
        } // if

        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        if (NSApp == nil) {
            [NSApplication sharedApplication];

            //if ([NSApp mainMenu] == nil) {
            //    CreateApplicationMenus();
            //}
            [NSApp finishLaunching];
        }
        [pool release];
    } // if

    return pnativefn_sdl12_SDL_Init(flags);
} // mactrampoline_sdl12_SDL_Init

// SDL's loadso functions have to go through our dlopen() trampoline.
//  Otherwise, it can't load ELFs, respect our overrides, and have trampolines
//  to align the stack, etc.
static void *mactrampoline_sdl12_SDL_LoadObject(const char *soname)
{
    void *retval = mactrampoline_dlopen(soname, RTLD_NOW);
    if (!retval)
        pnativefn_sdl12_SDL_SetError("dlopen('%s') failed: %s", soname, mactrampoline_dlerror());
    return retval;
} // mactrampoline_sdl12_SDL_LoadObject

static void *mactrampoline_sdl12_SDL_LoadFunction(void *lib, const char *sym)
{
    void *retval = mactrampoline_dlsym(lib, sym);
    if (!retval)
        pnativefn_sdl12_SDL_SetError("dlsym('%s') failed: %s", sym, mactrampoline_dlerror());
    return retval;
} // mactrampoline_sdl12_SDL_LoadFunction

static void mactrampoline_sdl12_SDL_UnloadObject(void *lib)
{
    mactrampoline_dlclose(lib);
} // mactrampoline_sdl12_SDL_UnloadObject

// Let us use our native override for libGL (this would need to be more
//  robust if Apple had a real OpenGL GetProcAddress function instead of just
//  letting you dlsym() their library).
static void *gllib = NULL;
static int mactrampoline_sdl12_SDL_GL_LoadLibrary(const char *soname)
{
    if (!pnativefn_sdl12_SDL_WasInit(SDL_INIT_VIDEO))
    {
		pnativefn_sdl12_SDL_SetError("%s", "Video subsystem has not been initialized");
        return -1;
    } // if

    // we ignore (soname); you always get Apple's OpenGL (but we need to
    //  trampoline everything, of course).
    if (gllib == NULL)
        gllib = mactrampoline_sdl12_SDL_LoadObject("libGL.so.1");  // will trigger our native override.
    return gllib ? 0 : -1;
} // mactrampoline_sdl12_SDL_GL_LoadLibrary

static void *mactrampoline_sdl12_SDL_GL_GetProcAddress(const char *sym)
{
    if (gllib == NULL)
    {
        pnativefn_sdl12_SDL_SetError("%s", "No GL driver has been loaded");
        return NULL;
    } // if

    return mactrampoline_sdl12_SDL_LoadFunction(gllib, sym);
} // mactrampoline_sdl12_SDL_GL_GetProcAddress

static void mactrampoline_sdl12_SDL_SetError(const char *fmt, ...)
{
    // we had to override just because this has varargs.
    va_list ap;
    va_start(ap, fmt);
    char *str = NULL;
    const int rc = vasprintf(&str, fmt, ap);
    va_end(ap);
    if ((rc >= 0) && (str != NULL))  // if not, oh well.
        pnativefn_sdl12_SDL_SetError("%s", str);
    free(str);
} // mactrampoline_sdl12_SDL_SetError

static int mactrampoline_sdl12_SDL_GetWMInfo(void *unused)
{
    // This is only going to cause problems, so force it off.
    //  Probably a lot of Unix apps that assume this has X11 info.
    pnativefn_sdl12_SDL_SetError("%s", "SDL_GetWMInfo not supported");
    return 0;
} // mactrampoline_sdl12_SDL_GetWMInfo


void *load_native_sdl12(void)
{
    const char *libname = "libSDL-1.2.0.dylib";
    void *handle = dlopen(libname, RTLD_NOW);
    if (handle == NULL)
        fprintf(stderr, "WARNING: tried to load native SDL 1.2 and failed: %s\n", dlerror());
    else
    {
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            pnativefn_sdl12_##fn = (nativefntype_sdl12_##fn) dlsym(handle, #fn); \
            if (pnativefn_sdl12_##fn == NULL) { \
                warn_missing_native_symbol(libname, #fn); \
            } else { \
                insert_symbol(#fn, mactrampoline_sdl12_##fn); \
            } \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_sdl12.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE
    } // else

    return handle;
} // load_native_sdl12

void unload_native_sdl12(void *handle)
{
    if (handle != NULL)
    {
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            remove_symbol(#fn); \
            pnativefn_sdl12_##fn = NULL; \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_sdl12.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE

        dlclose(handle);
    } // if
} // unload_native_sdl12

#endif

// end of mactrampolines_sdl12.m ...

