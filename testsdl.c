#include "SDL.h"
#include "mojoelf.h"

// For expedience, we just #include the .c file.
#define MOJOELF_SUPPORT_DLERROR 1
#define MOJOELF_SUPPORT_DLOPEN_FILE 1
#define MOJOELF_ALLOW_SYSTEM_RESOLVE 1
#define MOJOELF_REDUCE_LIBC_DEPENDENCIES 1
#include "mojoelf.c"

typedef int (*fnSDL_Init)(Uint32);
typedef SDL_Surface *(*fnSDL_SetVideoMode)(int, int, int, Uint32);
typedef int (*fnSDL_FillRect)(SDL_Surface *, SDL_Rect *, Uint32);
typedef int (*fnSDL_Flip)(SDL_Surface *);
typedef void (*fnSDL_Quit)(void);
typedef Uint32 (*fnSDL_MapRGB)(SDL_PixelFormat *, Uint8, Uint8, Uint8);

int main(int argc, const char **argv)
{
    SDL_Surface *screen = NULL;
    fnSDL_Init pSDL_Init = NULL;
    fnSDL_SetVideoMode pSDL_SetVideoMode = NULL;
    fnSDL_FillRect pSDL_FillRect = NULL;
    fnSDL_Flip pSDL_Flip = NULL;
    fnSDL_Quit pSDL_Quit = NULL;
    fnSDL_MapRGB pSDL_MapRGB = NULL;
    void *lib = NULL;

    lib = MOJOELF_dlopen_file("/usr/lib/libSDL-1.2.so.0", NULL);
    if (lib == NULL)
    {
        fprintf(stderr, "Failed to load SDL: %s\n", MOJOELF_dlerror());
        return 1;
    } // if

    printf("Loaded SDL.\n");

    #define LOOKUP_SYM(x) { \
        p##x = (fn##x) MOJOELF_dlsym(lib, #x); \
        if (p##x == NULL) { \
            fprintf(stderr, "Lookup %s failed: %s\n", #x, MOJOELF_dlerror()); \
            MOJOELF_dlclose(lib); \
            return 1; \
        } \
        printf("Found sym '%s': %p\n", #x, p##x); \
    }

    LOOKUP_SYM(SDL_Init);
    LOOKUP_SYM(SDL_SetVideoMode);
    LOOKUP_SYM(SDL_FillRect);
    LOOKUP_SYM(SDL_Flip);
    LOOKUP_SYM(SDL_Quit);
    LOOKUP_SYM(SDL_MapRGB);

    #undef LOOKUP_SYM

    printf("Calling SDL_Init().\n");
    if (pSDL_Init(SDL_INIT_VIDEO) == -1)
        fprintf(stderr, "SDL_Init() failed.\n");
    else
    {
        printf("Calling SDL_SetVideoMode(). You should get a window.\n");
        screen = pSDL_SetVideoMode(640, 480, 32, 0);
        if (screen == NULL)
            fprintf(stderr, "SDL_SetVideoMode() failed.\n");
        else
        {
            printf("Calling SDL_FillRect().\n");
            pSDL_FillRect(screen, NULL, pSDL_MapRGB(screen->format, 0xFF,0,0));
            printf("Calling SDL_Flip()...window contents should turn red.\n");
            pSDL_Flip(screen);
            printf("Sit here 10 seconds.\n");
            sleep(10);
        } // else

        printf("Calling SDL_Quit().\n");
        pSDL_Quit();
    } // else

    printf("Closing SDL library.\n");
    MOJOELF_dlclose(lib);

    printf("All done!\n");
    return 0;
} // main

// end of testsdl.c ...

