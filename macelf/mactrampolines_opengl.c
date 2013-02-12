/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// Native overrides for OpenGL...

// You absolutely must build this file with -mstackrealign.
//  A lot of these functions are here only as a trampoline: the ELF code calls
//  it, it aligns the stack to 16 bytes thanks to -mstackrealign, and then
//  calls the actual function in the system library.

#include "macelf.h"

#if MACELF_SUPPORT_NATIVE_OVERRIDE_OPENGL

#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <GL/glx.h>

// Deal with stuff that might not be in the Mac headers (yet).
#ifndef GL_NV_half_float
typedef unsigned short GLhalfNV;
#endif
#ifndef GL_OES_fixed_point
typedef GLint GLfixed;
#endif
#ifndef GL_ARB_cl_event
struct _cl_context;
struct _cl_event;
#endif
#ifndef GL_ARB_debug_output
typedef void (*GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam);
#endif
#ifndef GL_AMD_debug_output
typedef void (*GLDEBUGPROCAMD)(GLuint id,GLenum category,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam);
#endif
#ifndef GL_KHR_debug
typedef void (*GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam);
#endif
#ifndef GL_NV_vdpau_interop
typedef GLintptr GLvdpauSurfaceNV;
#endif


#define MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    typedef typ (*nativefntype_opengl_##fn) params; \
    static nativefntype_opengl_##fn pnativefn_opengl_##fn = NULL;
#define MACTRAMPOLINE(typ,fn,params,args,ret) \
    MACTRAMPOLINE_OVERRIDE(typ,fn,params) \
    static typ mactrampoline_opengl_##fn params { \
        ret pnativefn_opengl_##fn args; \
    }
#include "mactrampolines_opengl.h"
#include "mactrampolines_glx.h"
#undef MACTRAMPOLINE_OVERRIDE
#undef MACTRAMPOLINE

void *load_native_opengl(void)
{
    const int wantglx = GWantGLX;
    const char *nativelib = "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
    const char *glxlib = "/usr/X11/lib/libGL.1.dylib";
    const char *libname = wantglx ? glxlib : nativelib;
    void *handle = dlopen(libname, RTLD_NOW);
    if (handle == NULL)
    {
        fprintf(stderr, "WARNING: tried to load native %s and failed: %s\n", wantglx ? "glX" : "OpenGL", dlerror());
        if (wantglx)
            fprintf(stderr, "WARNING:  You might need to install Xquartz from http://xquartz.macosforge.org/\n");
    } // if
    else
    {
        // if a symbol is missing, we don't care. We cast a wide net, and many are expected to not exist on Mac OS X.
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            pnativefn_opengl_##fn = (nativefntype_opengl_##fn) dlsym(handle, #fn); \
            if (pnativefn_opengl_##fn != NULL) { \
                insert_symbol(#fn, mactrampoline_opengl_##fn); \
            } \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_opengl.h"
        if (wantglx) {
            #include "mactrampolines_glx.h"
        }
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE
    } // else

    return handle;
} // load_native_opengl

void unload_native_opengl(void *handle)
{
    if (handle != NULL)
    {
        #define MACTRAMPOLINE(typ,fn,params,args,ret) { \
            remove_symbol(#fn); \
            pnativefn_opengl_##fn = NULL; \
        }
        #define MACTRAMPOLINE_OVERRIDE(typ,fn,params) MACTRAMPOLINE(typ,fn,params,XXX,XXX);
        #include "mactrampolines_opengl.h"
        #include "mactrampolines_glx.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_OVERRIDE

        dlclose(handle);
    } // if
} // unload_native_opengl

#endif

// end of mactrampolines_opengl.c ...

