/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file was written by Ryan C. Gordon.
 */

// Do not #pragma once this file, it's intentionally included multiple times.

MACTRAMPOLINE(XVisualInfo*,glXChooseVisual,(Display *a, int b, int *c),(a,b,c),return)
MACTRAMPOLINE(GLXContext,glXCreateContext,(Display *a, XVisualInfo *b, GLXContext c, Bool d),(a,b,c,d),return)
MACTRAMPOLINE(void,glXDestroyContext,(Display *a, GLXContext b),(a,b),)
MACTRAMPOLINE(Bool,glXMakeCurrent,(Display *a, GLXDrawable b, GLXContext c),(a,b,c),return)
MACTRAMPOLINE(void,glXCopyContext,(Display *a, GLXContext b, GLXContext c, unsigned long d),(a,b,c,d),)
MACTRAMPOLINE(void,glXSwapBuffers,(Display *a, GLXDrawable b),(a,b),)
MACTRAMPOLINE(GLXPixmap,glXCreateGLXPixmap,(Display *a, XVisualInfo *b, Pixmap c),(a,b,c),return)
MACTRAMPOLINE(void,glXDestroyGLXPixmap,(Display *a, GLXPixmap b),(a,b),)
MACTRAMPOLINE(Bool,glXQueryExtension,(Display *a, int *b, int *c),(a,b,c),return)
MACTRAMPOLINE(Bool,glXQueryVersion,(Display *a, int *b, int *c),(a,b,c),return)
MACTRAMPOLINE(Bool,glXIsDirect,(Display *a, GLXContext b),(a,b),return)
MACTRAMPOLINE(int,glXGetConfig,(Display *a, XVisualInfo *b, int c, int *d),(a,b,c,d),return)
MACTRAMPOLINE(GLXContext,glXGetCurrentContext,(void),(),return)
MACTRAMPOLINE(GLXDrawable,glXGetCurrentDrawable,(void),(),return)
MACTRAMPOLINE(void,glXWaitGL,(void),(),)
MACTRAMPOLINE(void,glXWaitX,(void),(),)
MACTRAMPOLINE(void,glXUseXFont,(Font a, int b, int c, int d),(a,b,c,d),)
MACTRAMPOLINE(const char *,glXQueryExtensionsString,(Display *a, int b),(a,b),return)
MACTRAMPOLINE(const char *,glXQueryServerString,(Display *a, int b, int c),(a,b,c),return)
MACTRAMPOLINE(const char *,glXGetClientString,(Display *a, int b),(a,b),return)
MACTRAMPOLINE(Display *,glXGetCurrentDisplay,(void),(),return)
MACTRAMPOLINE(GLXFBConfig *,glXChooseFBConfig,(Display *a, int b, const int *c, int *d),(a,b,c,d),return)
MACTRAMPOLINE(int,glXGetFBConfigAttrib,(Display *a, GLXFBConfig b, int c, int *d),(a,b,c,d),return)
MACTRAMPOLINE(GLXFBConfig *,glXGetFBConfigs,(Display *a, int b, int *c),(a,b,c),return)
MACTRAMPOLINE(XVisualInfo *,glXGetVisualFromFBConfig,(Display *a, GLXFBConfig b),(a,b),return)
MACTRAMPOLINE(GLXWindow,glXCreateWindow,(Display *a, GLXFBConfig b, Window c, const int *d),(a,b,c,d),return)
MACTRAMPOLINE(void,glXDestroyWindow,(Display *a, GLXWindow b),(a,b),)
MACTRAMPOLINE(GLXPixmap,glXCreatePixmap,(Display *a, GLXFBConfig b, Pixmap c, const int *d),(a,b,c,d),return)
MACTRAMPOLINE(void,glXDestroyPixmap,(Display *a, GLXPixmap b),(a,b),)
MACTRAMPOLINE(GLXPbuffer,glXCreatePbuffer,(Display *a, GLXFBConfig b, const int *c),(a,b,c),return)
MACTRAMPOLINE(void,glXDestroyPbuffer,(Display *a, GLXPbuffer b),(a,b),)
MACTRAMPOLINE(void,glXQueryDrawable,(Display *a, GLXDrawable b, int c, unsigned int *d),(a,b,c,d),)
MACTRAMPOLINE(GLXContext,glXCreateNewContext,(Display *a, GLXFBConfig b, int c, GLXContext d, Bool e),(a,b,c,d,e),return)
MACTRAMPOLINE(Bool,glXMakeContextCurrent,(Display *a, GLXDrawable b, GLXDrawable c, GLXContext d),(a,b,c,d),return)
MACTRAMPOLINE(GLXDrawable,glXGetCurrentReadDrawable,(void),(),return)
MACTRAMPOLINE(int,glXQueryContext,(Display *a, GLXContext b, int c, int *d),(a,b,c,d),return)
MACTRAMPOLINE(void,glXSelectEvent,(Display *a, GLXDrawable b, unsigned long c),(a,b,c),)
MACTRAMPOLINE(void,glXGetSelectedEvent,(Display *a, GLXDrawable b, unsigned long *c),(a,b,c),)
MACTRAMPOLINE(__GLXextFuncPtr,glXGetProcAddressARB,(const GLubyte *a),(a),return)
MACTRAMPOLINE(__GLXextFuncPtr,glXGetProcAddress,(const GLubyte *a),(a),return)
MACTRAMPOLINE(void*,glXAllocateMemoryNV,(GLsizei a, GLfloat b, GLfloat c, GLfloat d),(a,b,c,d),return)
MACTRAMPOLINE(void,glXFreeMemoryNV,(GLvoid *a),(a),)
MACTRAMPOLINE(Bool,glXBindTexImageARB,(Display *a, GLXPbuffer b, int c),(a,b,c),return)
MACTRAMPOLINE(Bool,glXReleaseTexImageARB,(Display *a, GLXPbuffer b, int c),(a,b,c),return)
MACTRAMPOLINE(Bool,glXDrawableAttribARB,(Display *a, GLXDrawable b, const int *c),(a,b,c),return)
MACTRAMPOLINE(int,glXGetFrameUsageMESA,(Display *a, GLXDrawable b, float *c),(a,b,c),return)
MACTRAMPOLINE(int,glXBeginFrameTrackingMESA,(Display *a, GLXDrawable b),(a,b),return)
MACTRAMPOLINE(int,glXEndFrameTrackingMESA,(Display *a, GLXDrawable b),(a,b),return)
MACTRAMPOLINE(int,glXQueryFrameTrackingMESA,(Display *a, GLXDrawable b, int64_t *c, int64_t *d, float *e),(a,b,c,d,e),return)
MACTRAMPOLINE(int,glXSwapIntervalMESA,(unsigned int a),(a),return)
MACTRAMPOLINE(int,glXGetSwapIntervalMESA,(void),(),return)
MACTRAMPOLINE(void,glXBindTexImageEXT,(Display *a, GLXDrawable b, int c, const int *d),(a,b,c,d),)
MACTRAMPOLINE(void,glXReleaseTexImageEXT,(Display *a, GLXDrawable b, int c),(a,b,c),)

// end of mactrampolines_glx.h ...

