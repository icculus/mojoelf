/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// Do not #pragma once this file, it's intentionally included multiple times.

//MACTRAMPOLINE(void *,SDL_LoadObject,(const char *a),(a),return)
//MACTRAMPOLINE(void *,SDL_LoadFunction,(void *a, const char *b),(a,b),return)
//MACTRAMPOLINE(void,SDL_UnloadObject,(void *a),(a),)
//MACTRAMPOLINE(int,SDL_GL_LoadLibrary,(const char *a),(a),return)
//MACTRAMPOLINE(void *,SDL_GL_GetProcAddress,(const char *a),(a),return)
//MACTRAMPOLINE(void,SDL_SetError,(const char *fmt, ...)

//MACTRAMPOLINE(int,SDL_Init,(Uint32 a),(a),return)

MACTRAMPOLINE(int,SDL_InitSubSystem,(Uint32 a),(a),return)
MACTRAMPOLINE(void,SDL_QuitSubSystem,(Uint32 a),(a),)
MACTRAMPOLINE(Uint32,SDL_WasInit,(Uint32 a),(a),return)
MACTRAMPOLINE(void,SDL_Quit,(void),(),)
MACTRAMPOLINE(Uint8,SDL_GetAppState,(void),(),return)
MACTRAMPOLINE(int,SDL_AudioInit,(const char *a),(a),return)
MACTRAMPOLINE(void,SDL_AudioQuit,(void),(),)
MACTRAMPOLINE(char *,SDL_AudioDriverName,(char *a, int b),(a,b),return)
MACTRAMPOLINE(int,SDL_OpenAudio,(SDL_AudioSpec *a, SDL_AudioSpec *b),(a,b),return)
MACTRAMPOLINE(SDL_audiostatus,SDL_GetAudioStatus,(void),(),return)
MACTRAMPOLINE(void,SDL_PauseAudio,(int a),(a),)
MACTRAMPOLINE(SDL_AudioSpec *,SDL_LoadWAV_RW,(SDL_RWops *a, int b, SDL_AudioSpec *c, Uint8 **d, Uint32 *e),(a,b,c,d,e),return)
MACTRAMPOLINE(void,SDL_FreeWAV,(Uint8 *a),(a),)
MACTRAMPOLINE(int,SDL_BuildAudioCVT,(SDL_AudioCVT *a, Uint16 b, Uint8 c, int d, Uint16 e, Uint8 f, int g),(a,b,c,d,e,f,g),return)
MACTRAMPOLINE(int,SDL_ConvertAudio,(SDL_AudioCVT *a),(a),return)
MACTRAMPOLINE(void,SDL_MixAudio,(Uint8 *a, const Uint8 *b, Uint32 c, int d),(a,b,c,d),)
MACTRAMPOLINE(void,SDL_LockAudio,(void),(),)
MACTRAMPOLINE(void,SDL_UnlockAudio,(void),(),)
MACTRAMPOLINE(void,SDL_CloseAudio,(void),(),)
MACTRAMPOLINE(int,SDL_CDNumDrives,(void),(),return)
MACTRAMPOLINE(const char *,SDL_CDName,(int a),(a),return)
MACTRAMPOLINE(SDL_CD *,SDL_CDOpen,(int a),(a),return)
MACTRAMPOLINE(CDstatus,SDL_CDStatus,(SDL_CD *a),(a),return)
MACTRAMPOLINE(int,SDL_CDPlayTracks,(SDL_CD *a,int b, int c, int d, int e),(a,b,c,d,e),return)
MACTRAMPOLINE(int,SDL_CDPlay,(SDL_CD *a, int b, int c),(a,b,c),return)
MACTRAMPOLINE(int,SDL_CDPause,(SDL_CD *a),(a),return)
MACTRAMPOLINE(int,SDL_CDResume,(SDL_CD *a),(a),return)
MACTRAMPOLINE(int,SDL_CDStop,(SDL_CD *a),(a),return)
MACTRAMPOLINE(int,SDL_CDEject,(SDL_CD *a),(a),return)
MACTRAMPOLINE(void,SDL_CDClose,(SDL_CD *a),(a),)
MACTRAMPOLINE(SDL_bool,SDL_HasRDTSC,(void),(),return)
MACTRAMPOLINE(SDL_bool,SDL_HasMMX,(void),(),return)
MACTRAMPOLINE(SDL_bool,SDL_HasMMXExt,(void),(),return)
MACTRAMPOLINE(SDL_bool,SDL_Has3DNow,(void),(),return)
MACTRAMPOLINE(SDL_bool,SDL_Has3DNowExt,(void),(),return)
MACTRAMPOLINE(SDL_bool,SDL_HasSSE,(void),(),return)
MACTRAMPOLINE(SDL_bool,SDL_HasSSE2,(void),(),return)
MACTRAMPOLINE(SDL_bool,SDL_HasAltiVec,(void),(),return)
MACTRAMPOLINE(char *,SDL_GetError,(void),(),return)
MACTRAMPOLINE(void,SDL_ClearError,(void),(),)
MACTRAMPOLINE(void,SDL_Error,(SDL_errorcode a),(a),)
MACTRAMPOLINE(void,SDL_PumpEvents,(void),(),)
MACTRAMPOLINE(int,SDL_PeepEvents,(SDL_Event *a, int b, SDL_eventaction c, Uint32 d),(a,b,c,d),return)
MACTRAMPOLINE(int,SDL_PollEvent,(SDL_Event *a),(a),return)
MACTRAMPOLINE(int,SDL_WaitEvent,(SDL_Event *a),(a),return)
MACTRAMPOLINE(int,SDL_PushEvent,(SDL_Event *a),(a),return)
MACTRAMPOLINE(void,SDL_SetEventFilter,(SDL_EventFilter a),(a),)
MACTRAMPOLINE(SDL_EventFilter,SDL_GetEventFilter,(void),(),return)
MACTRAMPOLINE(Uint8,SDL_EventState,(Uint8 a, int b),(a,b),return)
MACTRAMPOLINE(int,SDL_NumJoysticks,(void),(),return)
MACTRAMPOLINE(const char *,SDL_JoystickName,(int a),(a),return)
MACTRAMPOLINE(SDL_Joystick *,SDL_JoystickOpen,(int a),(a),return)
MACTRAMPOLINE(int,SDL_JoystickOpened,(int a),(a),return)
MACTRAMPOLINE(int,SDL_JoystickIndex,(SDL_Joystick *a),(a),return)
MACTRAMPOLINE(int,SDL_JoystickNumAxes,(SDL_Joystick *a),(a),return)
MACTRAMPOLINE(int,SDL_JoystickNumBalls,(SDL_Joystick *a),(a),return)
MACTRAMPOLINE(int,SDL_JoystickNumHats,(SDL_Joystick *a),(a),return)
MACTRAMPOLINE(int,SDL_JoystickNumButtons,(SDL_Joystick *a),(a),return)
MACTRAMPOLINE(void,SDL_JoystickUpdate,(void),(),return)
MACTRAMPOLINE(int,SDL_JoystickEventState,(int a),(a),return)
MACTRAMPOLINE(Sint16,SDL_JoystickGetAxis,(SDL_Joystick *a, int b),(a,b),return)
MACTRAMPOLINE(Uint8,SDL_JoystickGetHat,(SDL_Joystick *a, int b),(a,b),return)
MACTRAMPOLINE(int,SDL_JoystickGetBall,(SDL_Joystick *a, int b, int *c, int *d),(a,b,c,d),return)
MACTRAMPOLINE(Uint8,SDL_JoystickGetButton,(SDL_Joystick *a, int b),(a,b),return)
MACTRAMPOLINE(void,SDL_JoystickClose,(SDL_Joystick *a),(a),)
MACTRAMPOLINE(int,SDL_EnableUNICODE,(int a),(a),return)
MACTRAMPOLINE(int,SDL_EnableKeyRepeat,(int a, int b),(a,b),return)
MACTRAMPOLINE(void,SDL_GetKeyRepeat,(int *a, int *b),(a,b),)
MACTRAMPOLINE(Uint8 *,SDL_GetKeyState,(int *a),(a),return)
MACTRAMPOLINE(SDLMod,SDL_GetModState,(void),(),return)
MACTRAMPOLINE(void,SDL_SetModState,(SDLMod a),(a),)
MACTRAMPOLINE(char *,SDL_GetKeyName,(SDLKey a),(a),return)
MACTRAMPOLINE(Uint8,SDL_GetMouseState,(int *a, int *b),(a,b),return)
MACTRAMPOLINE(Uint8,SDL_GetRelativeMouseState,(int *a, int *b),(a,b),return)
MACTRAMPOLINE(void,SDL_WarpMouse,(Uint16 a, Uint16 b),(a,b),)
MACTRAMPOLINE(SDL_Cursor *,SDL_CreateCursor,(Uint8 *a, Uint8 *b, int c, int d, int e, int f),(a,b,c,d,e,f),return)
MACTRAMPOLINE(void,SDL_SetCursor,(SDL_Cursor *a),(a),)
MACTRAMPOLINE(SDL_Cursor *,SDL_GetCursor,(void),(),return)
MACTRAMPOLINE(void,SDL_FreeCursor,(SDL_Cursor *a),(a),)
MACTRAMPOLINE(int,SDL_ShowCursor,(int a),(a),return)
MACTRAMPOLINE(SDL_mutex *,SDL_CreateMutex,(void),(),return)
MACTRAMPOLINE(int,SDL_mutexP,(SDL_mutex *a),(a),return)
MACTRAMPOLINE(int,SDL_mutexV,(SDL_mutex *a),(a),return)
MACTRAMPOLINE(void,SDL_DestroyMutex,(SDL_mutex *a),(a),)
MACTRAMPOLINE(SDL_sem *,SDL_CreateSemaphore,(Uint32 a),(a),return)
MACTRAMPOLINE(void,SDL_DestroySemaphore,(SDL_sem *a),(a),)
MACTRAMPOLINE(int,SDL_SemWait,(SDL_sem *a),(a),return)
MACTRAMPOLINE(int,SDL_SemTryWait,(SDL_sem *a),(a),return)
MACTRAMPOLINE(int,SDL_SemWaitTimeout,(SDL_sem *a, Uint32 b),(a,b),return)
MACTRAMPOLINE(int,SDL_SemPost,(SDL_sem *a),(a),return)
MACTRAMPOLINE(Uint32,SDL_SemValue,(SDL_sem *a),(a),return)
MACTRAMPOLINE(SDL_cond *,SDL_CreateCond,(void),(),return)
MACTRAMPOLINE(void,SDL_DestroyCond,(SDL_cond *a),(a),)
MACTRAMPOLINE(int,SDL_CondSignal,(SDL_cond *a),(a),return)
MACTRAMPOLINE(int,SDL_CondBroadcast,(SDL_cond *a),(a),return)
MACTRAMPOLINE(int,SDL_CondWait,(SDL_cond *a, SDL_mutex *b),(a,b),return)
MACTRAMPOLINE(int,SDL_CondWaitTimeout,(SDL_cond *a, SDL_mutex *b, Uint32 c),(a,b,c),return)
MACTRAMPOLINE(SDL_RWops *,SDL_RWFromFile,(const char *a, const char *b),(a,b),return);
MACTRAMPOLINE(SDL_RWops *,SDL_RWFromFP,(FILE *a, int b),(a,b),return)
MACTRAMPOLINE(SDL_RWops *,SDL_RWFromMem,(void *a, int b),(a,b),return)
MACTRAMPOLINE(SDL_RWops *,SDL_RWFromConstMem,(const void *a, int b),(a,b),return)
MACTRAMPOLINE(SDL_RWops *,SDL_AllocRW,(void),(),return)
MACTRAMPOLINE(void,SDL_FreeRW,(SDL_RWops *a),(a),)
MACTRAMPOLINE(Uint16,SDL_ReadLE16,(SDL_RWops *a),(a),return)
MACTRAMPOLINE(Uint16,SDL_ReadBE16,(SDL_RWops *a),(a),return)
MACTRAMPOLINE(Uint32,SDL_ReadLE32,(SDL_RWops *a),(a),return)
MACTRAMPOLINE(Uint32,SDL_ReadBE32,(SDL_RWops *a),(a),return)
MACTRAMPOLINE(Uint64,SDL_ReadLE64,(SDL_RWops *a),(a),return)
MACTRAMPOLINE(Uint64,SDL_ReadBE64,(SDL_RWops *a),(a),return)
MACTRAMPOLINE(int,SDL_WriteLE16,(SDL_RWops *a, Uint16 b),(a,b),return)
MACTRAMPOLINE(int,SDL_WriteBE16,(SDL_RWops *a, Uint16 b),(a,b),return)
MACTRAMPOLINE(int,SDL_WriteLE32,(SDL_RWops *a, Uint32 b),(a,b),return)
MACTRAMPOLINE(int,SDL_WriteBE32,(SDL_RWops *a, Uint32 b),(a,b),return)
MACTRAMPOLINE(int,SDL_WriteLE64,(SDL_RWops *a, Uint64 b),(a,b),return)
MACTRAMPOLINE(int,SDL_WriteBE64,(SDL_RWops *a, Uint64 b),(a,b),return)
MACTRAMPOLINE(SDL_iconv_t,SDL_iconv_open,(const char *a, const char *b),(a,b),return)
MACTRAMPOLINE(int,SDL_iconv_close,(SDL_iconv_t a),(a),return)
MACTRAMPOLINE(size_t,SDL_iconv,(SDL_iconv_t a, const char **b, size_t *c, char **d, size_t *e),(a,b,c,d,e),return)
MACTRAMPOLINE(char *,SDL_iconv_string,(const char *a, const char *b, const char *c, size_t d),(a,b,c,d),return)
MACTRAMPOLINE(char*,SDL_lltoa,(Sint64 a, char *b, int c),(a,b,c),return)
MACTRAMPOLINE(char *,SDL_ltoa,(long a, char *b, int c),(a,b,c),return)
MACTRAMPOLINE(char *,SDL_strrev,(char *a),(a),return)
MACTRAMPOLINE(char *,SDL_strupr,(char *a),(a),return)
MACTRAMPOLINE(char *,SDL_strlwr,(char *a),(a),return)
MACTRAMPOLINE(char *,SDL_ultoa,(unsigned long a, char *b, int c),(a,b,c),return)
MACTRAMPOLINE(char*,SDL_ulltoa,(Uint64 a, char *b, int c),(a,b,c),return)
MACTRAMPOLINE(SDL_Thread *,SDL_CreateThread,(int (*a)(void *), void *b),(a,b),return)
MACTRAMPOLINE(Uint32,SDL_ThreadID,(void),(),return)
MACTRAMPOLINE(Uint32,SDL_GetThreadID,(SDL_Thread *a),(a),return)
MACTRAMPOLINE(void,SDL_WaitThread,(SDL_Thread *a, int *b),(a,b),)
MACTRAMPOLINE(void,SDL_KillThread,(SDL_Thread *a),(a),)
MACTRAMPOLINE(Uint32,SDL_GetTicks,(void),(),return)
MACTRAMPOLINE(void,SDL_Delay,(Uint32 a),(a),)
MACTRAMPOLINE(int,SDL_SetTimer,(Uint32 a, SDL_TimerCallback b),(a,b),return)
MACTRAMPOLINE(SDL_TimerID,SDL_AddTimer,(Uint32 a, SDL_NewTimerCallback b, void *c),(a,b,c),return)
MACTRAMPOLINE(SDL_bool,SDL_RemoveTimer,(SDL_TimerID a),(a),return)
MACTRAMPOLINE(const SDL_version *,SDL_Linked_Version,(void),(),return)
MACTRAMPOLINE(int,SDL_VideoInit,(const char *a, Uint32 b),(a,b),return)
MACTRAMPOLINE(void,SDL_VideoQuit,(void),(),)
MACTRAMPOLINE(char *,SDL_VideoDriverName,(char *a, int b),(a,b),return)
MACTRAMPOLINE(SDL_Surface *,SDL_GetVideoSurface,(void),(),return)
MACTRAMPOLINE(const SDL_VideoInfo *,SDL_GetVideoInfo,(void),(),return)
MACTRAMPOLINE(int,SDL_VideoModeOK,(int a, int b, int c, Uint32 d),(a,b,c,d),return)
MACTRAMPOLINE(SDL_Rect **,SDL_ListModes,(SDL_PixelFormat *a, Uint32 b),(a,b),return)
MACTRAMPOLINE(SDL_Surface *,SDL_SetVideoMode,(int a, int b, int c, Uint32 d),(a,b,c,d),return)
MACTRAMPOLINE(void,SDL_UpdateRects,(SDL_Surface *a, int b, SDL_Rect *c),(a,b,c),)
MACTRAMPOLINE(void,SDL_UpdateRect,(SDL_Surface *a, Sint32 b, Sint32 c, Uint32 d, Uint32 e),(a,b,c,d,e),)
MACTRAMPOLINE(int,SDL_Flip,(SDL_Surface *a),(a),return)
MACTRAMPOLINE(int,SDL_SetGamma,(float a, float b, float c),(a,b,c),return)
MACTRAMPOLINE(int,SDL_SetGammaRamp,(const Uint16 *a, const Uint16 *b, const Uint16 *c),(a,b,c),return)
MACTRAMPOLINE(int,SDL_GetGammaRamp,(Uint16 *a, Uint16 *b, Uint16 *c),(a,b,c),return)
MACTRAMPOLINE(int,SDL_SetColors,(SDL_Surface *a, SDL_Color *b, int c, int d),(a,b,c,d),return)
MACTRAMPOLINE(int,SDL_SetPalette,(SDL_Surface *a, int b, SDL_Color *c, int d, int e),(a,b,c,d,e),return)
MACTRAMPOLINE(Uint32,SDL_MapRGB,(const SDL_PixelFormat * const a, const Uint8 b, const Uint8 c, const Uint8 d),(a,b,c,d),return)
MACTRAMPOLINE(Uint32,SDL_MapRGBA,(const SDL_PixelFormat * const a, const Uint8 b, const Uint8 c, const Uint8 d, const Uint8 e),(a,b,c,d,e),return)
MACTRAMPOLINE(void,SDL_GetRGB,(Uint32 a, const SDL_PixelFormat * const b, Uint8 *c, Uint8 *d, Uint8 *e),(a,b,c,d,e),)
MACTRAMPOLINE(void,SDL_GetRGBA,(Uint32 a, const SDL_PixelFormat * const b, Uint8 *c, Uint8 *d, Uint8 *e, Uint8 *f),(a,b,c,d,e,f),)
MACTRAMPOLINE(SDL_Surface *,SDL_CreateRGBSurface,(Uint32 a, int b, int c, int d, Uint32 e, Uint32 f, Uint32 g, Uint32 h),(a,b,c,d,e,f,g,h),return)
MACTRAMPOLINE(SDL_Surface *,SDL_CreateRGBSurfaceFrom,(void *a, int b, int c, int d, int e, Uint32 f, Uint32 g, Uint32 h, Uint32 i),(a,b,c,d,e,f,g,h,i),return)
MACTRAMPOLINE(void,SDL_FreeSurface,(SDL_Surface *a),(a),)
MACTRAMPOLINE(int,SDL_LockSurface,(SDL_Surface *a),(a),return)
MACTRAMPOLINE(void,SDL_UnlockSurface,(SDL_Surface *a),(a),)
MACTRAMPOLINE(SDL_Surface *,SDL_LoadBMP_RW,(SDL_RWops *a, int b),(a,b),return)
MACTRAMPOLINE(int,SDL_SaveBMP_RW,(SDL_Surface *a, SDL_RWops *b, int c),(a,b,c),return)
MACTRAMPOLINE(int,SDL_SetColorKey,(SDL_Surface *a, Uint32 b, Uint32 c),(a,b,c),return)
MACTRAMPOLINE(int,SDL_SetAlpha,(SDL_Surface *a, Uint32 b, Uint8 c),(a,b,c),return)
MACTRAMPOLINE(SDL_bool,SDL_SetClipRect,(SDL_Surface *a, const SDL_Rect *b),(a,b),return)
MACTRAMPOLINE(void,SDL_GetClipRect,(SDL_Surface *a, SDL_Rect *b),(a,b),)
MACTRAMPOLINE(SDL_Surface *,SDL_ConvertSurface,(SDL_Surface *a, SDL_PixelFormat *b, Uint32 c),(a,b,c),return)
MACTRAMPOLINE(int,SDL_UpperBlit,(SDL_Surface *a, SDL_Rect *b, SDL_Surface *c, SDL_Rect *d),(a,b,c,d),return)
MACTRAMPOLINE(int,SDL_LowerBlit,(SDL_Surface *a, SDL_Rect *b, SDL_Surface *c, SDL_Rect *d),(a,b,c,d),return)
MACTRAMPOLINE(int,SDL_FillRect,(SDL_Surface *a, SDL_Rect *b, Uint32 c),(a,b,c),return)
MACTRAMPOLINE(SDL_Surface *,SDL_DisplayFormat,(SDL_Surface *a),(a),return)
MACTRAMPOLINE(SDL_Surface *,SDL_DisplayFormatAlpha,(SDL_Surface *a),(a),return)
MACTRAMPOLINE(SDL_Overlay *,SDL_CreateYUVOverlay,(int a, int b, Uint32 c, SDL_Surface *d),(a,b,c,d),return)
MACTRAMPOLINE(int,SDL_LockYUVOverlay,(SDL_Overlay *a),(a),return)
MACTRAMPOLINE(void,SDL_UnlockYUVOverlay,(SDL_Overlay *a),(a),)
MACTRAMPOLINE(int,SDL_DisplayYUVOverlay,(SDL_Overlay *a, SDL_Rect *b),(a,b),return)
MACTRAMPOLINE(void,SDL_FreeYUVOverlay,(SDL_Overlay *a),(a),)
MACTRAMPOLINE(int,SDL_GL_SetAttribute,(SDL_GLattr a, int b),(a,b),return)
MACTRAMPOLINE(int,SDL_GL_GetAttribute,(SDL_GLattr a, int *b),(a,b),return)
MACTRAMPOLINE(void,SDL_GL_SwapBuffers,(void),(),)
MACTRAMPOLINE(void,SDL_GL_UpdateRects,(int a, SDL_Rect* b),(a,b),)
MACTRAMPOLINE(void,SDL_GL_Lock,(void),(),)
MACTRAMPOLINE(void,SDL_GL_Unlock,(void),(),)
MACTRAMPOLINE(void,SDL_WM_SetCaption,(const char *a, const char *b),(a,b),)
MACTRAMPOLINE(void,SDL_WM_GetCaption,(char **a, char **b),(a,b),)
MACTRAMPOLINE(void,SDL_WM_SetIcon,(SDL_Surface *a, Uint8 *b),(a,b),)
MACTRAMPOLINE(int,SDL_WM_IconifyWindow,(void),(),return)
MACTRAMPOLINE(int,SDL_WM_ToggleFullScreen,(SDL_Surface *a),(a),return)
MACTRAMPOLINE(SDL_GrabMode,SDL_WM_GrabInput,(SDL_GrabMode a),(a),return)
MACTRAMPOLINE(int,SDL_SoftStretch,(SDL_Surface *a, SDL_Rect *b, SDL_Surface *c, SDL_Rect *d),(a,b,c,d),return)

// end of mactrampolines_sdl12.h ...

