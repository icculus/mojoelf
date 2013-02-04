// You absolutely must build this file with -mstackrealign.
//  A lot of these functions are here only as a trampoline: the ELF code calls
//  it, it aligns the stack to 16 bytes thanks to -mstackrealign, and then
//  calls the actual function in the system library.

#define _DARWIN_USE_64_BIT_INODE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <complex.h>
#include <iconv.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <locale.h>
#include <getopt.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <utime.h>

#include "mojoelf.h"

extern char **environ;  // !!! FIXME: really? This isn't in a header?

#define STUBBED(x) do { \
    static int seen_this = 0; \
    if (!seen_this) { \
        seen_this = 1; \
        fprintf(stderr, "STUBBED: %s at %s (%s:%d)\n", x, __FUNCTION__, __FILE__, __LINE__); \
    } \
} while (0)


#define MACTRAMPOLINE_OVERRIDE(fn)

#define MACTRAMPOLINE(typ,fn,params,args,ret) \
    static typ mactrampoline_##fn params { ret fn args; }

// Some extra work is needed for trampolines that use varargs.
#define MACTRAMPOLINE_PRINTF(fn, params, args) \
    static int mactrampoline_##fn params { \
        va_list ap; \
        va_start(ap, fmt); \
        const int retval = v##fn args; \
        va_end(ap); \
        return retval; \
    }

#include "mactrampolines.h"

#undef MACTRAMPOLINE
#undef MACTRAMPOLINE_PRINTF
#undef MACTRAMPOLINE_OVERRIDE


// NOTE: pthread_t is a pointer on Mac OS X, but a long int on Linux (so it
// NOTE:  happens to be pointer-sized on x86 and amd64).



// Wrap Linux's errno (Which is actually __errno_location()).
static int *mactrampoline___errno_location(void)
{
    STUBBED("convert errno from Mac values to Linux");
    return &errno;
} // mactrampoline___errno_location

// this is what an ELF's _start entry point (probably) calls.
static int mactrampoline___libc_start_main(
    int (*main) (int, char **, char **), int argc, char ** argv,
	void (*init) (void), void (*fini) (void), void (*rtld_fini) (void),
	void *stack_end)
{
    if (fini != NULL)
        atexit(fini);
    if (rtld_fini != NULL)
        atexit(rtld_fini);
    if (init != NULL)
        init();

    const int retval = main(argc, argv, environ);
    exit(retval);
} // mactrampoline___libc_start_main

static int mactrampoline___cxa_atexit(void (*func) (void *), void * arg, void * dso_handle)
{
    STUBBED("write me");
    return 0;
} // mactrampoline___cxa_atexit

static size_t mactrampoline___ctype_get_mb_cur_max(void)
{
    STUBBED("I have no idea");
    return 4;
} // mactrampoline___ctype_get_mb_cur_max


static int mactrampoline___fprintf_chk(FILE *io, int flag, const char *fmt, ...)
{
    STUBBED("check flag (and stack!)");
    va_list ap;
    va_start(ap, fmt);
    const int retval = vfprintf(io, fmt, ap);
    va_end(ap);
    return retval;
} // mactrampoline___fprintf_chk

static int mactrampoline___printf_chk(int flag, const char *fmt, ...)
{
    STUBBED("check flag (and stack!)");
    va_list ap;
    va_start(ap, fmt);
    const int retval = vprintf(fmt, ap);
    va_end(ap);
    return retval;
} // mactrampoline___printf_chk

static int mactrampoline___sprintf_chk(char *buf, int flag, size_t buflen, const char *fmt, ...)
{
    STUBBED("check flag (and stack!)");
    STUBBED("this should abort if buflen == 0");
    va_list ap;
    va_start(ap, fmt);
    const int retval = vsnprintf(buf, buflen, fmt, ap);
    va_end(ap);
    return retval;
} // mactrampoline___sprintf_chk

static int mactrampoline___vsnprintf_chk(char *buf, size_t maxbuflen, int flag, size_t buflen, const char *fmt, va_list ap)
{
    STUBBED("check flag (and stack!)");
    STUBBED("this should abort if buflen < maxbuflen");
    const int retval = vsnprintf(buf, buflen, fmt, ap);
    return retval;
} // mactrampoline___vsnprintf_chk

static char *mactrampoline___strcat_chk(char *dst, const char *src, size_t len)
{
    STUBBED("check flag (and stack!)");
    return strcat(dst, src);
} // __strcat_chk

static void mactrampoline___stack_chk_fail(void)
{
    fprintf(stderr, "__stack_chk_fail!\n");
    fprintf(stderr, "Corrupted stack detected, aborting!\n\n");
    _exit(1);
} // mactrampoline___stack_chk_fail

// Just use the "locked" versions for now, since the unlocked don't exist.
static int mactrampoline_fputs_unlocked(const char *str, FILE *io)
{
    return fputs(str, io);
} // mactrampoline_fputs_unlocked

// Just use the "locked" versions for now, since the unlocked don't exist.
static size_t mactrampoline_fwrite_unlocked(const void *ptr, size_t size, size_t num, FILE *io)
{
    return fwrite(ptr, size, num, io);
} // mactrampoline_fwrite_unlocked

static size_t mactrampoline___fpending(FILE *io)
{
    STUBBED("write me");
    return 0;
} // mactrampoline___fpending

extern char *program_invocation_name;
static void mactrampoline_error(int status, int errnum, const char *fmt, ...)
{
    STUBBED("there are other global vars this function checks");
    va_list ap;
    fflush(stdout);
    fprintf(stderr, "%s: ", program_invocation_name);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (errnum)
        fprintf(stderr, ": %s", strerror(errnum));
    fprintf(stderr, "\n");
    fflush(stderr);
    if (status != 0)
        exit(status);
} // mactrampoline_error


// mode_t is 2 bytes on Mac OS X, but 4 on Linux.
// dev_t is 4 bytes on Mac OS X, but 8 on Linux.
static int mactrampoline_creat(const char* a, /*mode_t*/uint32_t b)
{
    return creat(a, (mode_t) b);
} // mactrampoline_creat

static int mactrampoline_shm_open(const char* a, int b, /*mode_t*/uint32_t c)
{
    return shm_open(a, b, (mode_t) c);
} // mactrampoline_shm_open

static int mactrampoline_chmod(const char* a, /*mode_t*/uint32_t b)
{
    return chmod(a, (mode_t) b);
} // mactrampoline_chmod

static int mactrampoline_fchmod(int a, /*mode_t*/uint32_t b)
{
    return fchmod(a, (mode_t) b);
} // mactrampoline_fchmod

static int mactrampoline_mkdir(const char* a, /*mode_t*/uint32_t b)
{
    return mkdir(a, (mode_t) b);
} // mactrampoline_mkdir

static int mactrampoline_mkfifo(const char* a, /*mode_t*/uint32_t b)
{
    return mkfifo(a, (mode_t) b);
} // mactrampoline_mkfifo

static int mactrampoline_mknod(const char* a, /*mode_t*/uint32_t b, /*dev_t*/uint64_t c)
{
    return mknod(a, (mode_t) b, (dev_t) c);
} // mactrampoline_mknod

static /*mode_t*/uint32_t mactrampoline_umask(/*mode_t*/uint32_t a)
{
    return (uint32_t) umask((mode_t) a);
} // mactrampoline_umask


// off_t is 32-bit on x86 Linux, 64-bit on amd64 Linux, 64-bit everywhere on Mac OS X.
static int mactrampoline_ftruncate(int fd, /*off_t*/uintptr_t len)
{
    return ftruncate(fd, (off_t) len);
} // mactrampoline_ftruncate

static /*off_t*/uintptr_t mactrampoline_lseek(int fd, /*off_t*/uintptr_t offset, int whence)
{
    return (uintptr_t) lseek(fd, (off_t) offset, whence);
} // mactrampoline_lseek

static ssize_t mactrampoline_pread(int fd, void *buf, size_t len, /*off_t*/uintptr_t offset)
{
    return pread(fd, buf, len, (off_t) offset);
} // mactrampoline_pread

static ssize_t mactrampoline_pwrite(int fd, const void* buf, size_t len, /*off_t*/uintptr_t offset)
{
    return pwrite(fd, buf, len, (off_t) offset);
} // mactrampoline_pwrite

static int mactrampoline_truncate(const char* path, /*off_t*/uintptr_t offset)
{
    return truncate(path, (off_t) offset);
} // mactrampoline_truncate

static void *mactrampoline_mmap(void* addr, size_t len, int prot, int flags, int fd, /*off_t*/uintptr_t offset)
{
    return mmap(addr, len, prot, flags, fd, (off_t) offset);
} // mactrampoline_mmap

static int mactrampoline_fseeko(FILE* io, /*off_t*/uintptr_t offset, int whence)
{
    return fseeko(io, (off_t) offset, whence);
} // mactrampoline_fseeko

static /*off_t*/uintptr_t mactrampoline_ftello(FILE* io)
{
    return (uintptr_t) ftello(io);
} // mactrampoline_ftello


// gettext support functions...
static char *mactrampoline_bindtextdomain(const char *domain, const char *dir)
{
    STUBBED("write me");
    return NULL;
} // mactrampoline_bindtextdomain

static char *mactrampoline_textdomain(const char *domain)
{
    STUBBED("write me");
    return NULL;
} // mactrampoline_textdomain

static char *mactrampoline_dcgettext(const char *domain, const char *msgid, int category)
{
    STUBBED("write me");
    return (char *) msgid;
} // mactrampoline_dcgettext

static const unsigned short **mactrampoline___ctype_b_loc(void)
{
    STUBBED("write me");
    STUBBED("this should be thread-local, too");
    return NULL;
} // mactrampoline___ctype_b_loc

// mbstate_t is totally different on Mac and Linux.
//  On Linux, it's always 8 bytes, on Mac, it's a whole big thing.
static int mactrampoline_mbsinit(/*mbstate_t*/uint64_t *mbstate)
{
    STUBBED("write me");
    return (*mbstate == 0);
} // mactrampoline_mbsinit


#ifdef st_atime
#undef st_atime
#endif
#ifdef st_mtime
#undef st_mtime
#endif
#ifdef st_ctime
#undef st_ctime
#endif

#ifdef __i386__
typedef struct linux_stat32
{
    uint32_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;
    uint32_t st_atime;
    uint32_t st_atime_nsec;
    uint32_t st_mtime;
    uint32_t st_mtime_nsec;
    uint32_t st_ctime;
    uint32_t st_ctime_nsec;
    uint32_t __unused4;
    uint32_t __unused5;
} linux_stat32;

typedef struct linux_stat64
{
    uint64_t st_dev;
    uint32_t __pad0;
    uint32_t __st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint32_t __pad3;
    int64_t st_size;
    uint32_t st_blksize;
    uint64_t st_blocks;
    uint32_t st_atime;
    uint32_t st_atime_nsec;
    uint32_t st_mtime;
    uint32_t st_mtime_nsec;
    uint32_t st_ctime;
    uint32_t st_ctime_nsec;
    uint64_t st_ino;
} linux_stat64;

#elif defined(__x86_64__)

typedef struct linux_stat64
{
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_nlink;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t __pad0;
    uint64_t st_rdev;
    int64_t st_size;
    int64_t st_blksize;
    int64_t st_blocks;	/* Number 512-byte blocks allocated. */
    uint64_t st_atime;
    uint64_t st_atime_nsec;
    uint64_t st_mtime;
    uint64_t st_mtime_nsec;
    uint64_t st_ctime;
    uint64_t st_ctime_nsec;
    int64_t unused[3];
} linux_stat64;

#else
#error Please define your platform.
#endif

#ifdef __i386__
static void mac_stat_to_linux64(struct stat *macstat, linux_stat64 *lnxstat)
{
// mode_t is 2 bytes on Mac OS X, but 4 on Linux.
// dev_t is 4 bytes on Mac OS X, but 8 on Linux.

    lnxstat->st_dev = (uint64_t) macstat->st_dev;
    lnxstat->__pad0 = 0;
    lnxstat->__st_ino = (uint32_t) macstat->st_ino;
    lnxstat->st_mode = (uint32_t) macstat->st_mode;
    lnxstat->st_nlink = (uint32_t) macstat->st_nlink;
    lnxstat->st_uid = (uint32_t) macstat->st_uid;
    lnxstat->st_gid = (uint32_t) macstat->st_gid;
    lnxstat->st_rdev = (uint64_t) macstat->st_rdev;
    lnxstat->__pad3 = 0;
    lnxstat->st_size = (int64_t) macstat->st_size;
    lnxstat->st_blksize = (uint32_t) macstat->st_blksize;
    lnxstat->st_blocks = (uint64_t) macstat->st_blocks;
    lnxstat->st_atime = (uint32_t) macstat->st_atimespec.tv_sec;
    lnxstat->st_atime_nsec = (uint32_t) macstat->st_atimespec.tv_nsec;
    lnxstat->st_mtime = (uint32_t) macstat->st_mtimespec.tv_sec;
    lnxstat->st_mtime_nsec = (uint32_t) macstat->st_mtimespec.tv_nsec;
    lnxstat->st_ctime = (uint32_t) macstat->st_ctimespec.tv_sec;
    lnxstat->st_ctime_nsec = (uint32_t) macstat->st_ctimespec.tv_nsec;
    lnxstat->st_ino = (uint64_t) macstat->st_ino;
}

#define XSTAT_IMPL(fn, arg) \
    struct stat macstat; \
    assert(ver == 3); \
    if (fn(arg, &macstat) == -1) { \
        return -1; \
    } \
    mac_stat_to_linux64(&macstat, lnxstat); \
    return 0;

static int mactrampoline___xstat64(int ver, const char *path, linux_stat64 *lnxstat)
{
    XSTAT_IMPL(stat, path);
} // mactrampoline___fxstat64

static int mactrampoline___lxstat64(int ver, const char *path, linux_stat64 *lnxstat)
{
    XSTAT_IMPL(lstat, path);
} // mactrampoline___fxstat64

static int mactrampoline___fxstat64(int ver, int fd, linux_stat64 *lnxstat)
{
    XSTAT_IMPL(fstat, fd);
} // mactrampoline___fxstat64
#endif


static char *mactrampoline___strdup(const char *str) { return strdup(str); }


typedef enum
{
    LINUX_LC_CTYPE = 0,
    LINUX_LC_NUMERIC = 1,
    LINUX_LC_TIME = 2,
    LINUX_LC_COLLATE = 3,
    LINUX_LC_MONETARY = 4,
    LINUX_LC_MESSAGES = 5,
    LINUX_LC_ALL = 6,
} LinuxLocaleCategory;

// Linux uses different values for locale categories, so map them.
static char *mactrampoline_setlocale(int category, const char *locale)
{
    int maccat = 0;
    switch (category)
    {
        #define CVTTOMACLOCALE(cat) case LINUX_##cat: maccat = cat; break;
        CVTTOMACLOCALE(LC_CTYPE);
        CVTTOMACLOCALE(LC_NUMERIC);
        CVTTOMACLOCALE(LC_TIME);
        CVTTOMACLOCALE(LC_COLLATE);
        CVTTOMACLOCALE(LC_MONETARY);
        CVTTOMACLOCALE(LC_MESSAGES);
        CVTTOMACLOCALE(LC_ALL);
        #undef CVTTOMACLOCALE

        default: STUBBED("Missing locale category?"); return NULL;
    } // switch

    return setlocale(maccat, locale);
} // mactrampoline_setlocale



// We only need a mutex for our global state during the
//  dlopen/dlsym/dlclone() trampolines. Initial loading happens on the main
//  (and, at the time, only) thread before handing control to the ELF code.
static pthread_mutex_t loader_mutex;

// Obviously we want to map dlopen and friends through MojoELF. We can't let
//  you talk to Mach-O binaries directly in any case, due to calling
//  convention differences.
extern const MOJOELF_Callbacks mojoelf_callbacks;  // !!! FIXME: booo
static void *mactrampoline_dlopen(const char *fname, int flags)
{
    STUBBED("trap a few libs like SDL, OpenGL, X11, OpenAL...");
    STUBBED("flags are ignored");
    pthread_mutex_lock(&loader_mutex);
    void *retval = MOJOELF_dlopen_file(fname, &mojoelf_callbacks);
    pthread_mutex_unlock(&loader_mutex);
    return retval;
} // mactrampoline_dlopen

static void *mactrampoline_dlsym(void *lib, const char *sym)
{
    pthread_mutex_lock(&loader_mutex);
    void *retval = MOJOELF_dlsym(lib, sym);
    pthread_mutex_unlock(&loader_mutex);
    return retval;
} // mactrampoline_dlsym

static int mactrampoline_dlclose(void *lib)
{
    pthread_mutex_lock(&loader_mutex);
    MOJOELF_dlclose(lib);
    pthread_mutex_unlock(&loader_mutex);
    return 0;
} // mactrampoline_dlclose

static char *mactrampoline_dlerror(void)
{
    STUBBED("This should make a temp copy if this is really non-const");
    return (char *) MOJOELF_dlerror();
} // mactrampoline_dlerror


static int mactrampoline_ioctl(int fd, int req, ...)
{
    STUBBED("this is going to need a bunch of complexity");
    fprintf(stderr, "WARNING: unhandled ioctl(%d, %d, ...) called!\n", fd, req);
    return -1;
} // mactrampoline_ioctl


typedef struct LinuxDirEnt32
{
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];
} LinuxDirEnt32;

typedef struct LinuxDirEnt64
{
    uint64_t d_ino;
    uint64_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];
} LinuxDirEnt64;

typedef struct LinuxDIR
{
    DIR *macdir;
    union
    {
        LinuxDirEnt32 dent32;
        LinuxDirEnt64 dent64;
    };
} LinuxDIR;

static LinuxDIR *mactrampoline_opendir(const char *path)
{
    LinuxDIR *retval = NULL;
    DIR *macdir = opendir(path);
    if (macdir)
    {
        retval = (LinuxDIR *) malloc(sizeof (LinuxDIR));
        if (retval == NULL)
            closedir(macdir);
        else
        {
            memset(retval, '\0', sizeof (*retval));
            retval->macdir = macdir;
        } // else
    } // if

    return retval;
} // mactrampoline_opendir

static void mactrampoline_rewinddir(LinuxDIR *dir)
{
    rewinddir(dir->macdir);
} // mactrampoline_rewinddir

static void mactrampoline_seekdir(LinuxDIR *dir, long pos)
{
    seekdir(dir->macdir, pos);
} // mactrampoline_seekdir

static long mactrampoline_telldir(LinuxDIR *dir)
{
    return telldir(dir->macdir);
} // mactrampoline_telldir

static int mactrampoline_readdir64_r(LinuxDIR *dir, LinuxDirEnt64 *lnxdent, LinuxDirEnt64 **result)
{
    struct dirent macdent;
    struct dirent *macresult = NULL;

    memset(&macdent, '\0', sizeof (macdent));
    const int rc = readdir_r(dir->macdir, &macdent, &macresult);

    if (!macresult)
        *result = NULL;
    else
    {
        assert(macresult == &macdent);
        *result = lnxdent;
        lnxdent->d_ino = (uint64_t) macdent.d_ino;
        lnxdent->d_off = (uint64_t) macdent.d_seekoff;
        lnxdent->d_reclen = (uint16_t) macdent.d_reclen;
        lnxdent->d_type = (uint8_t) macdent.d_type;  // these all match up.

        if ((macdent.d_namlen+1) >= sizeof (lnxdent->d_name))
            STUBBED("What should we do here?");
        snprintf(lnxdent->d_name, sizeof (lnxdent->d_name), "%s", macdent.d_name);

        *result = lnxdent;
    } // else

    return rc;
} // mactrampoline_readdir64_r

static LinuxDirEnt64 *mactrampoline_readdir64(LinuxDIR *dir)
{
    LinuxDirEnt64 *retval = NULL;
    return (mactrampoline_readdir64_r(dir, &dir->dent64, &retval) == -1) ? NULL : retval;
} // mactrampoline_readdir64


#ifdef __i386__
static int mactrampoline_readdir_r(LinuxDIR *dir, LinuxDirEnt32 *lnxdent, LinuxDirEnt32 **result)
{
    struct dirent macdent;
    struct dirent *macresult = NULL;

    memset(&macdent, '\0', sizeof (macdent));
    const int rc = readdir_r(dir->macdir, &macdent, &macresult);

    if (!macresult)
        *result = NULL;
    else
    {
        assert(macresult == &macdent);
        *result = lnxdent;
        lnxdent->d_ino = (uint32_t) macdent.d_ino;
        lnxdent->d_off = (uint32_t) macdent.d_seekoff;
        lnxdent->d_reclen = (uint16_t) macdent.d_reclen;
        lnxdent->d_type = (uint8_t) macdent.d_type;  // these all match up.

        if ((macdent.d_namlen+1) >= sizeof (lnxdent->d_name))
            STUBBED("What should we do here?");
        snprintf(lnxdent->d_name, sizeof (lnxdent->d_name), "%s", macdent.d_name);

        *result = lnxdent;
    } // else

    return rc;
} // mactrampoline_readdir_r

static LinuxDirEnt32 *mactrampoline_readdir(LinuxDIR *dir)
{
    LinuxDirEnt32 *retval = NULL;
    return (mactrampoline_readdir_r(dir, &dir->dent32, &retval) == -1) ? NULL : retval;
} // mactrampoline_readdir

#else

static int mactrampoline_readdir_r(LinuxDIR *dir, LinuxDirEnt64 *lnxdent, LinuxDirEnt64 **result)
{
    return mactrampoline_readdir64_r(dir, lnxdent, result);
} // mactrampoline_readdir_r

static LinuxDirEnt64 *mactrampoline_readdir(LinuxDIR *dir)
{
    return mactrampoline_readdir64(dir);
} // mactrampoline_readdir
#endif

static int mactrampoline_closedir(LinuxDIR *dir)
{
    const int rc = closedir(dir->macdir);
    if (rc == 0)
        free(dir);
    return rc;
} // mactrampoline_closedir


typedef enum
{
    LINUX_O_RDONLY      =00,
    LINUX_O_WRONLY      =01,
    LINUX_O_RDWR        =02,
    LINUX_O_CREAT       =0100,
    LINUX_O_EXCL        =0200,
    LINUX_O_NOCTTY      =0400,
    LINUX_O_TRUNC       =01000,
    LINUX_O_APPEND      =02000,
    LINUX_O_NONBLOCK    =04000,
    LINUX_O_ASYNC       =020000,
    LINUX_O_DIRECT      =040000,
    LINUX_O_LARGEFILE   =0100000,
    LINUX_O_DIRECTORY   =0200000,
    LINUX_O_NOFOLLOW    =0400000,
    LINUX_O_NOATIME     =01000000,
    LINUX_O_CLOEXEC     =02000000,
    LINUX_O_SYNC        =04010000,
} LinuxOpenFlags;

static int mactrampoline_open(const char *path, int flags, ...)
{
    int macflags = 0;
    mode_t mode = 0;

    // Have to use varargs if O_CREAT, grumble grumble.
    if (flags & LINUX_O_CREAT)
    {
        va_list ap;
        va_start(ap, flags);
        mode = (mode_t) va_arg(ap, uint32_t);  // mode_t is 4 bytes on Linux, 2 on Mac.
        va_end(ap);
    } // if

    // Linux flags aren't the same values, so map them.
    // O_RDONLY is zero in both of them, though.
    #define MAPFLAG(fl) if (flags & LINUX_##fl) { flags &= ~LINUX_##fl; macflags |= fl; }
    MAPFLAG(O_WRONLY);
    MAPFLAG(O_RDWR);
    MAPFLAG(O_CREAT);
    MAPFLAG(O_EXCL);
    MAPFLAG(O_NOCTTY);
    MAPFLAG(O_TRUNC);
    MAPFLAG(O_APPEND);
    MAPFLAG(O_NONBLOCK);
    MAPFLAG(O_ASYNC);
    //MAPFLAG(O_DIRECT);
    MAPFLAG(O_DIRECTORY);
    MAPFLAG(O_NOFOLLOW);
    //MAPFLAG(O_NOATIME);
    MAPFLAG(O_CLOEXEC);
    MAPFLAG(O_SYNC);
    #undef MAPFLAG

    #ifdef __i386__
    if (flags & LINUX_O_LARGEFILE)
    {
        STUBBED("need anything for O_LARGEFILE?");
        flags &= ~LINUX_O_LARGEFILE;
    } // if
    #endif

    if (flags)  // uhoh, something we didn't handle!
    {
        errno = EPERM;
        return -1;
    } // if

    STUBBED("Look for some /proc and /dev entries and fake them");

    return open(path, macflags, mode);
} // mactrampoline_open

static int mactrampoline_open64(const char *path, int flags, ...)
{
    flags |= LINUX_O_LARGEFILE;

    // Have to use varargs if O_CREAT, grumble grumble.
    if (flags & LINUX_O_CREAT)
    {
        va_list ap;
        va_start(ap, flags);
        const mode_t mode = (mode_t) va_arg(ap, uint32_t);  // mode_t is 4 bytes on Linux, 2 on Mac.
        va_end(ap);
        return mactrampoline_open(path, flags, mode);
    } // if

    return mactrampoline_open(path, flags);
} // mactrampoline_open64

static FILE *mactrampoline_fopen64(const char *fname, const char *mode)
{
    return fopen(fname, mode);  // I think this is always 64-bit clean on Mac?
} // mactrampoline_fopen64

static int mactrampoline_fseeko64(FILE *io, uint64_t offset, int whence)
{
    return fseeko(io, (off_t) offset, whence); // off_t is always 64-bit on Mac OS X.
} // mactrampoline_fseeko64

static uint64_t mactrampoline_ftello64(FILE *io)
{
    return (uint64_t) ftello(io); // off_t is always 64-bit on Mac OS X.
} // mactrampoline_ftello64

static void mactrampoline___assert_fail(const char *assertion, const char *fname, unsigned int line, const char *fn)
{
    fflush(stdout);
    fflush(stderr);
    fprintf(stderr, "%s:%u: %s: Assertion %s failed.\n", fname, line, fn, assertion);
    fflush(stderr);
    abort();
} // mactrampoline___assert_fail


int insert_symbol(const char *fn, void *ptr);  // !!! FIXME: booo
int build_trampolines(void)
{
    pthread_mutex_init(&loader_mutex, NULL);

    return insert_symbol("stderr", stderr) &&
           insert_symbol("stdout", stdout) &&
           insert_symbol("stdin", stdin) &&
           insert_symbol("program_invocation_name", program_invocation_name)
        #define MACTRAMPOLINE(typ,fn,params,args,ret) \
            && (insert_symbol(#fn, mactrampoline_##fn))
        #define MACTRAMPOLINE_PRINTF(fn, params, args) \
            && (insert_symbol(#fn, mactrampoline_##fn))
        #define MACTRAMPOLINE_OVERRIDE(fn) \
            && (insert_symbol(#fn, mactrampoline_##fn))
        #include "mactrampolines.h"
        #undef MACTRAMPOLINE
        #undef MACTRAMPOLINE_PRINTF
        #undef MACTRAMPOLINE_OVERRIDE
    ;
} // build_trampolines


void missing_symbol_called(const char *missing_symbol)
{
    fflush(stdout);
    fflush(stderr);
    fprintf(stderr, "\n\nMissing symbol '%s' called!\n", missing_symbol);
    fprintf(stderr, "Aborting.\n\n\n");
    //STUBBED("output backtrace");
    fflush(stderr);
    _exit(1);
} // missing_symbol_called

// end of mactrampolines.c ...

