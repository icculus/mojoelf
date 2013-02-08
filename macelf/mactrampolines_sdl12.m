/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// Native overrides for SDL 1.2...

#include "macelf.h"

#if MACELF_SUPPORT_NATIVE_OVERRIDE_SDL12

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <dlfcn.h>

#include "SDL.h"


#define MACTRAMPOLINE(typ,fn,params,args,ret) \
    typedef typ (*nativefntype_sdl12_##fn) params; \
    static nativefntype_sdl12_##fn pnativefn_sdl12_##fn = NULL; \
    static typ mactrampoline_sdl12_##fn params { \
        ret pnativefn_sdl12_##fn args; \
    }
#include "mactrampolines_sdl12.h"
#undef MACTRAMPOLINE

// override a few things...

// SDL_Init needs a little startup code, since we don't use SDLmain.
typedef int (*nativefntype_sdl12_SDL_Init)(Uint32 flags);
static nativefntype_sdl12_SDL_Init pnativefn_sdl12_SDL_Init = NULL;
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


void *load_native_sdl12(void)
{
    const char *libname = "libSDL-1.2.0.dylib";
    void *handle = dlopen(libname, RTLD_NOW);
    if (handle == NULL)
        fprintf(stderr, "WARNING: tried to load native SDL 1.2 and failed: %s\n", dlerror());
    else
    {
        pnativefn_sdl12_SDL_Init = (nativefntype_sdl12_SDL_Init) dlsym(handle, "SDL_Init");
        insert_symbol("SDL_Init", mactrampoline_sdl12_SDL_Init);

        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            pnativefn_sdl12_##fn = (nativefntype_sdl12_##fn) dlsym(handle, #fn); \
            if (pnativefn_sdl12_##fn == NULL) { \
                warn_missing_native_symbol(libname, #fn); \
            } else { \
                insert_symbol(#fn, mactrampoline_sdl12_##fn); \
            } \
        }
        #include "mactrampolines_sdl12.h"
        #undef MACTRAMPOLINE
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
        #include "mactrampolines_sdl12.h"
        #undef MACTRAMPOLINE

        remove_symbol("SDL_Init");
        pnativefn_sdl12_SDL_Init = NULL;

        dlclose(handle);
    } // if
} // unload_native_sdl12

#endif

// end of mactrampolines_sdl12.c ...

