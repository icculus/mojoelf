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
#include <langinfo.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <dlfcn.h>

#include "macelf.h"

extern char **environ;  // !!! FIXME: really? This isn't in a header?

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

// NOTE: pid_t is 32-bit on 32/64 bit Linux and Mac OS X.



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
    if (init != NULL)
        init();

    const int retval = main(argc, argv, environ);

    if (rtld_fini != NULL)
        rtld_fini();

    if (fini != NULL)
        fini();

    exit(retval);
} // mactrampoline___libc_start_main

static int mactrampoline___cxa_atexit(void (*func) (void *), void * arg, void * dso_handle)
{
    STUBBED("write me");
    return 0;
} // mactrampoline___cxa_atexit

static void mactrampoline___cxa_finalize(void *handle)
{
    STUBBED("write me");
} // mactrampoline___cxa_finalize

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


typedef enum
{
    LINUX_ABDAY_1=131072,
    LINUX_ABDAY_2=131073,
    LINUX_ABDAY_3=131074,
    LINUX_ABDAY_4=131075,
    LINUX_ABDAY_5=131076,
    LINUX_ABDAY_6=131077,
    LINUX_ABDAY_7=131078,
    LINUX_DAY_1=131079,
    LINUX_DAY_2=131080,
    LINUX_DAY_3=131081,
    LINUX_DAY_4=131082,
    LINUX_DAY_5=131083,
    LINUX_DAY_6=131084,
    LINUX_DAY_7=131085,
    LINUX_ABMON_1=131086,
    LINUX_ABMON_2=131087,
    LINUX_ABMON_3=131088,
    LINUX_ABMON_4=131089,
    LINUX_ABMON_5=131090,
    LINUX_ABMON_6=131091,
    LINUX_ABMON_7=131092,
    LINUX_ABMON_8=131093,
    LINUX_ABMON_9=131094,
    LINUX_ABMON_10=131095,
    LINUX_ABMON_11=131096,
    LINUX_ABMON_12=131097,
    LINUX_MON_1=131098,
    LINUX_MON_2=131099,
    LINUX_MON_3=131100,
    LINUX_MON_4=131101,
    LINUX_MON_5=131102,
    LINUX_MON_6=131103,
    LINUX_MON_7=131104,
    LINUX_MON_8=131105,
    LINUX_MON_9=131106,
    LINUX_MON_10=131107,
    LINUX_MON_11=131108,
    LINUX_MON_12=131109,
    LINUX_AM_STR=131110,
    LINUX_PM_STR=131111,
    LINUX_D_T_FMT=131112,
    LINUX_D_FMT=131113,
    LINUX_T_FMT=131114,
    LINUX_T_FMT_AMPM=131115,
    LINUX_ERA=131116,
    LINUX_ERA_D_FMT=131118,
    LINUX_ALT_DIGITS=131119,
    LINUX_ERA_D_T_FMT=131120,
    LINUX_ERA_T_FMT=131121,
    LINUX___DATE_FMT=131180,
    LINUX_CODESET=14,
    LINUX_RADIXCHAR=65536,
    LINUX_THOUSEP=65537,
    LINUX_YESEXPR=327680,
    LINUX_NOEXPR=327681,
    LINUX_YESSTR=327682,
    LINUX_NOSTR=327683,
} LinuxNlItem;

static char *mactrampoline_nl_langinfo(LinuxNlItem item)
{
    nl_item macitem = CODESET;
    switch (item)
    {
        #define CVTTOMACNLITEM(it) case LINUX_##it: macitem = it; break
        CVTTOMACNLITEM(ABDAY_1);
        CVTTOMACNLITEM(ABDAY_2);
        CVTTOMACNLITEM(ABDAY_3);
        CVTTOMACNLITEM(ABDAY_4);
        CVTTOMACNLITEM(ABDAY_5);
        CVTTOMACNLITEM(ABDAY_6);
        CVTTOMACNLITEM(ABDAY_7);
        CVTTOMACNLITEM(DAY_1);
        CVTTOMACNLITEM(DAY_2);
        CVTTOMACNLITEM(DAY_3);
        CVTTOMACNLITEM(DAY_4);
        CVTTOMACNLITEM(DAY_5);
        CVTTOMACNLITEM(DAY_6);
        CVTTOMACNLITEM(DAY_7);
        CVTTOMACNLITEM(ABMON_1);
        CVTTOMACNLITEM(ABMON_2);
        CVTTOMACNLITEM(ABMON_3);
        CVTTOMACNLITEM(ABMON_4);
        CVTTOMACNLITEM(ABMON_5);
        CVTTOMACNLITEM(ABMON_6);
        CVTTOMACNLITEM(ABMON_7);
        CVTTOMACNLITEM(ABMON_8);
        CVTTOMACNLITEM(ABMON_9);
        CVTTOMACNLITEM(ABMON_10);
        CVTTOMACNLITEM(ABMON_11);
        CVTTOMACNLITEM(ABMON_12);
        CVTTOMACNLITEM(MON_1);
        CVTTOMACNLITEM(MON_2);
        CVTTOMACNLITEM(MON_3);
        CVTTOMACNLITEM(MON_4);
        CVTTOMACNLITEM(MON_5);
        CVTTOMACNLITEM(MON_6);
        CVTTOMACNLITEM(MON_7);
        CVTTOMACNLITEM(MON_8);
        CVTTOMACNLITEM(MON_9);
        CVTTOMACNLITEM(MON_10);
        CVTTOMACNLITEM(MON_11);
        CVTTOMACNLITEM(MON_12);
        CVTTOMACNLITEM(AM_STR);
        CVTTOMACNLITEM(PM_STR);
        CVTTOMACNLITEM(D_T_FMT);
        CVTTOMACNLITEM(D_FMT);
        CVTTOMACNLITEM(T_FMT);
        CVTTOMACNLITEM(T_FMT_AMPM);
        CVTTOMACNLITEM(ERA);
        CVTTOMACNLITEM(ERA_D_FMT);
        CVTTOMACNLITEM(ALT_DIGITS);
        CVTTOMACNLITEM(ERA_D_T_FMT);
        CVTTOMACNLITEM(ERA_T_FMT);
        CVTTOMACNLITEM(CODESET);
        CVTTOMACNLITEM(RADIXCHAR);
        CVTTOMACNLITEM(THOUSEP);
        CVTTOMACNLITEM(YESEXPR);
        CVTTOMACNLITEM(NOEXPR);
        #ifdef YESSTR
        CVTTOMACNLITEM(YESSTR);
        #endif
        #ifdef NOSTR
        CVTTOMACNLITEM(NOSTR);
        #endif
        #undef CVTTOMACNLITEM

        // !!! FIXME: is __DATE_FMT different than D_FMT?
        case LINUX___DATE_FMT: macitem = D_FMT; break;

        default:
            fprintf(stderr, "WARNING: Unknown nl_item %d\n", (int) item);
            return NULL;
    };

    return nl_langinfo(macitem);
} // mactrampoline_nl_langinfo


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
typedef struct LinuxStat32
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
} LinuxStat32;

typedef struct LinuxStat64
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
} LinuxStat64;

#define NATIVE_STAT_BITS 32

#elif defined(__x86_64__)

typedef struct LinuxStat64
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
    int64_t st_blocks;
    uint64_t st_atime;
    uint64_t st_atime_nsec;
    uint64_t st_mtime;
    uint64_t st_mtime_nsec;
    uint64_t st_ctime;
    uint64_t st_ctime_nsec;
    int64_t unused[3];
} LinuxStat64;

#define NATIVE_STAT_BITS 64

#else
#error Please define your platform.
#endif

#ifdef __i386__
static void mac_stat_to_linux32(struct stat *macstat, LinuxStat32 *lnxstat)
{
    lnxstat->st_dev = (uint32_t) macstat->st_dev;
    lnxstat->st_ino = (uint32_t) macstat->st_ino;
    lnxstat->st_mode = (uint16_t) macstat->st_mode;
    lnxstat->st_nlink = (uint16_t) macstat->st_nlink;
    lnxstat->st_uid = (uint16_t) macstat->st_uid;
    lnxstat->st_gid = (uint16_t) macstat->st_gid;
    lnxstat->st_rdev = (uint32_t) macstat->st_rdev;
    lnxstat->st_size = (uint32_t) macstat->st_size;
    lnxstat->st_blksize = (uint32_t) macstat->st_blksize;
    lnxstat->st_blocks = (uint32_t) macstat->st_blocks;
    lnxstat->st_atime = (uint32_t) macstat->st_atimespec.tv_sec;
    lnxstat->st_atime_nsec = (uint32_t) macstat->st_atimespec.tv_nsec;
    lnxstat->st_mtime = (uint32_t) macstat->st_mtimespec.tv_sec;
    lnxstat->st_mtime_nsec = (uint32_t) macstat->st_mtimespec.tv_nsec;
    lnxstat->st_ctime = (uint32_t) macstat->st_ctimespec.tv_sec;
    lnxstat->st_ctime_nsec = (uint32_t) macstat->st_ctimespec.tv_nsec;
    lnxstat->__unused4 = lnxstat->__unused5 = 0;
} // mac_stat_to_linux32
#endif

static void mac_stat_to_linux64(struct stat *macstat, LinuxStat64 *lnxstat)
{
// mode_t is 2 bytes on Mac OS X, but 4 on Linux.
// dev_t is 4 bytes on Mac OS X, but 8 on Linux.

#ifdef __i386__
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

#elif defined(__x86_64__)
    lnxstat->st_dev = (uint64_t) macstat->st_dev;
    lnxstat->st_ino = (uint64_t) macstat->st_ino;
    lnxstat->st_nlink = (uint64_t) macstat->st_nlink;
    lnxstat->st_mode = (uint32_t) macstat->st_mode;
    lnxstat->st_uid = (uint32_t) macstat->st_uid;
    lnxstat->st_gid = (uint32_t) macstat->st_gid;
    lnxstat->__pad0 = 0;
    lnxstat->st_rdev = (uint64_t) macstat->st_rdev;
    lnxstat->st_size = (int64_t) macstat->st_size;
    lnxstat->st_blksize = (int64_t) macstat->st_blksize;
    lnxstat->st_blocks = (int64_t) macstat->st_blocks;
    lnxstat->st_atime = (uint64_t) macstat->st_atimespec.tv_sec;
    lnxstat->st_atime_nsec = (uint64_t) macstat->st_atimespec.tv_nsec;
    lnxstat->st_mtime = (uint64_t) macstat->st_mtimespec.tv_sec;
    lnxstat->st_mtime_nsec = (uint64_t) macstat->st_mtimespec.tv_nsec;
    lnxstat->st_ctime = (uint64_t) macstat->st_ctimespec.tv_sec;
    lnxstat->st_ctime_nsec = (uint64_t) macstat->st_ctimespec.tv_nsec;
    lnxstat->unused[0] = lnxstat->unused[1] = lnxstat->unused[2] = 0;

#else
#error Please define your platform.
#endif
}

#define XSTAT_IMPL2(bits, fn, arg) \
    struct stat macstat; \
    assert(ver == 3); \
    if (fn(arg, &macstat) == -1) { \
        return -1; \
    } \
    mac_stat_to_linux##bits(&macstat, lnxstat); \
    return 0;

#define XSTAT_IMPL(bits, fn, arg) XSTAT_IMPL2(bits, fn, arg)

static int mactrampoline___xstat64(int ver, const char *path, LinuxStat64 *lnxstat)
{
    XSTAT_IMPL(64, stat, path);
} // mactrampoline___fxstat64

static int mactrampoline___lxstat64(int ver, const char *path, LinuxStat64 *lnxstat)
{
    XSTAT_IMPL(64, lstat, path);
} // mactrampoline___fxstat64

static int mactrampoline___fxstat64(int ver, int fd, LinuxStat64 *lnxstat)
{
    XSTAT_IMPL(64, fstat, fd);
} // mactrampoline___fxstat64

static int mactrampoline___xstat(int ver, const char *path, LinuxStat32 *lnxstat)
{
    XSTAT_IMPL(NATIVE_STAT_BITS, stat, path);
} // mactrampoline___xstat

static int mactrampoline___lxstat(int ver, const char *path, LinuxStat32 *lnxstat)
{
    XSTAT_IMPL(NATIVE_STAT_BITS, lstat, path);
} // mactrampoline___lxstat

static int mactrampoline___fxstat(int ver, int fd, LinuxStat32 *lnxstat)
{
    XSTAT_IMPL(NATIVE_STAT_BITS, fstat, fd);
} // mactrampoline___fxstat

static int mactrampoline_stat(int ver, const char *path, LinuxStat32 *lnxstat)
{
    XSTAT_IMPL(NATIVE_STAT_BITS, stat, path);
} // mactrampoline_stat

static int mactrampoline_lstat(int ver, const char *path, LinuxStat32 *lnxstat)
{
    XSTAT_IMPL(NATIVE_STAT_BITS, lstat, path);
} // mactrampoline_lstat

static int mactrampoline_fstat(int ver, int fd, LinuxStat32 *lnxstat)
{
    XSTAT_IMPL(NATIVE_STAT_BITS, fstat, fd);
} // mactrampoline_fstat


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


typedef struct
{
    char *pw_name;
    char *pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
} LinuxPasswd;

static void mac_passwd_to_linux(const struct passwd *macpw, LinuxPasswd *lnxpw)
{
    lnxpw->pw_name = macpw->pw_name;
    lnxpw->pw_passwd = macpw->pw_passwd;
    lnxpw->pw_uid = macpw->pw_uid;
    lnxpw->pw_gid = macpw->pw_gid;
    lnxpw->pw_gecos = macpw->pw_gecos;
    lnxpw->pw_dir = macpw->pw_dir;
    lnxpw->pw_shell = macpw->pw_shell;
} // mac_passwd_to_linux

static LinuxPasswd *mactrampoline_getpwnam(const char *name)
{
    static LinuxPasswd lnxpw;
    // !!! FIXME: bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    static char buf[512];
    struct passwd macpw;
    struct passwd *result = NULL;

    if ((getpwnam_r(name, &macpw, buf, sizeof (buf), &result) != 0) || (!result))
        return NULL;
    mac_passwd_to_linux(&macpw, &lnxpw);
    return &lnxpw;
} // mactrampoline_getpwnam

static LinuxPasswd *mactrampoline_getpwuid(uid_t uid)
{
    static LinuxPasswd lnxpw;
    // !!! FIXME: bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    static char buf[512];
    struct passwd macpw;
    struct passwd *result = NULL;

    if ((getpwuid_r(uid, &macpw, buf, sizeof (buf), &result) != 0) || (!result))
        return NULL;
    mac_passwd_to_linux(&macpw, &lnxpw);
    return &lnxpw;
} // mactrampoline_getpwuid

static int mactrampoline_getpwnam_r(const char *name, LinuxPasswd *lnxpw, char *buf, size_t bufsize, LinuxPasswd **lnxresult)
{
    struct passwd macpw;
    struct passwd *macresult = NULL;
    const int rc = getpwnam_r(name, &macpw, buf, bufsize, &macresult);
    if (lnxresult)
        *lnxresult = ((rc == 0) && (macresult)) ? lnxpw : NULL;
    mac_passwd_to_linux(&macpw, lnxpw);
    return rc;
} // mactrampoline_getpwnam_r

static int mactrampoline_getpwuid_r(uid_t uid, LinuxPasswd *lnxpw, char *buf, size_t bufsize, LinuxPasswd **lnxresult)
{
    struct passwd macpw;
    struct passwd *macresult = NULL;
    const int rc = getpwuid_r(uid, &macpw, buf, bufsize, &macresult);
    if (lnxresult)
        *lnxresult = ((rc == 0) && (macresult)) ? lnxpw : NULL;
    mac_passwd_to_linux(&macpw, lnxpw);
    return rc;
} // mactrampoline_getpwuid_r

static LinuxPasswd *mactrampoline_getpwent(void)
{
    static LinuxPasswd lnxpw;
    struct passwd *macpw = getpwent();
    if (macpw == NULL)
        return NULL;
    mac_passwd_to_linux(macpw, &lnxpw);
    return &lnxpw;
} // mactrampoline_getpwent

// !!! FIXME: want builtins!
//#define SINCOS_IMPL(suffix,typ) \
//    static void mactrampoline_sincos##suffix(typ x, typ *_sin, typ *_cos) { \
//        __builtin_sincos##suffix(x, _sin, _cos); \
//    }
#define SINCOS_IMPL(suffix,typ) \
    static void mactrampoline_sincos##suffix(typ x, typ *_sin, typ *_cos) { \
        *_sin = sin(x); *_cos = cos(x); \
    }
SINCOS_IMPL(,double)
SINCOS_IMPL(f,float)
SINCOS_IMPL(l,long double)
#undef SINCOS_IMPL


// We only need a mutex for our global state during the
//  dlopen/dlsym/dlclone() trampolines. Initial loading happens on the main
//  (and, at the time, only) thread before handing control to the ELF code.
static pthread_mutex_t loader_mutex;

// Obviously we want to map dlopen and friends through MojoELF. We can't let
//  you talk to Mach-O binaries directly in any case, due to calling
//  convention differences.
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

static pid_t mactrampoline_waitpid(pid_t pid, int *status, int options)
{
    // Linux also has WCONTINUED.
    const int macoptions = options & (WNOHANG | WUNTRACED);
    if (macoptions != options)
        fprintf(stderr, "WARNING: called waitpid(%d) with unsupported options: %d\n", (int) pid, (int) options);

    return waitpid(pid, status, macoptions);
} // mactrampoline_waitpid


// jmp_buf is totally different on Linux, but if you think of it as a generic
//  memory buffer, the Linux buffer is way larger than we need.
//  setjmp on Linux becomes _setjmp in the binary standard, but it doesn't
//  work like the Mac _setjmp().
static int mactrampoline__setjmp(jmp_buf env)
{
    return setjmp(env);
} // mactrampoline__setjmp

static void mactrampoline_longjmp(jmp_buf env, int val)
{
    longjmp(env, val);
} // mactrampoline_longjmp


// pthread_t matches up closely enough, but much of the rest of the pthread
//  structs don't, so we end up allocating the mac versions, and using the
//  the Linux-sized buffers to store a single pointer to our allocated object.
static int mactrampoline_pthread_attr_init(void/*pthread_attr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_attr_t *macattr = (pthread_attr_t *) malloc(sizeof (pthread_attr_t));
    if (macattr == NULL)
        return ENOMEM;  // !!! FIXME: convert error
    const int rc = pthread_attr_init(macattr);
    if (rc != 0)
        free(macattr);
    else
        *((void **) lnxattr) = macattr;
    return rc;
} // mactrampoline_pthread_attr_init

static int mactrampoline_pthread_attr_destroy(void/*pthread_attr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_attr_t *macattr = *(pthread_attr_t **) lnxattr;
    const int rc = pthread_attr_destroy(macattr);
    if (rc == 0)
        free(macattr);
    return rc;
} // mactrampoline_pthread_attr_destroy

#define PTHREAD_ATTR_TRAMPOLINE(state, typ) \
    static int mactrampoline_pthread_attr_get##state(const void/*pthread_attr_t*/ *lnxattr, typ *val) { \
        STUBBED("need to convert errors to Linux values"); \
        return pthread_attr_get##state(*(const pthread_attr_t **) lnxattr, val); \
    } \
    static int mactrampoline_pthread_attr_set##state(void/*pthread_attr_t*/ *lnxattr, typ val) { \
        STUBBED("need to convert errors to Linux values"); \
        return pthread_attr_set##state(*(pthread_attr_t **) lnxattr, val); \
    }

PTHREAD_ATTR_TRAMPOLINE(detachstate, int)
PTHREAD_ATTR_TRAMPOLINE(guardsize, size_t)
PTHREAD_ATTR_TRAMPOLINE(inheritsched, int)
PTHREAD_ATTR_TRAMPOLINE(schedpolicy, int)
PTHREAD_ATTR_TRAMPOLINE(scope, int)
PTHREAD_ATTR_TRAMPOLINE(stackaddr, void *)
PTHREAD_ATTR_TRAMPOLINE(stacksize, size_t)

#undef PTHREAD_ATTR_TRAMPOLINE


static int mactrampoline_pthread_cond_init(void/*pthread_cond_t*/ *lnxcond, const void /*pthread_condattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_cond_t *maccond = (pthread_cond_t *) malloc(sizeof (pthread_cond_t));
    if (maccond == NULL)
        return ENOMEM;  // !!! FIXME: convert error
    const pthread_condattr_t *macattr = lnxattr ? *(const pthread_condattr_t **) lnxattr : NULL;
    const int rc = pthread_cond_init(maccond, macattr);
    if (rc != 0)
        free(maccond);
    else
        *((void **) lnxcond) = maccond;
    return rc;
} // mactrampoline_pthread_cond_init

static int mactrampoline_pthread_cond_destroy(void/*pthread_cond_t*/ *lnxcond)
{
    STUBBED("need to convert errors to Linux values");
    pthread_cond_t *maccond = *(pthread_cond_t **) lnxcond;
    const int rc = pthread_cond_destroy(maccond);
    if (rc == 0)
        free(maccond);
    return rc;
} // mactrampoline_pthread_cond_destroy

static int mactrampoline_pthread_cond_broadcast(void/*pthread_cond_t*/ *lnxcond)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_cond_broadcast(*(pthread_cond_t **) lnxcond);
} // mactrampoline_pthread_cond_broadcast

static int mactrampoline_pthread_cond_signal(void/*pthread_cond_t*/ *lnxcond)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_cond_signal(*(pthread_cond_t **) lnxcond);
} // mactrampoline_pthread_cond_signal

static int mactrampoline_pthread_cond_timedwait(void/*pthread_cond_t*/ *lnxcond, void/*pthread_mutex_t*/ *lnxmutex, const struct timespec *tspec)
{
    // timespec should match between Linux and Mac OS X.
    STUBBED("need to convert errors to Linux values");
    pthread_cond_t *maccond = *(pthread_cond_t **) lnxcond;
    pthread_mutex_t *macmutex = *(pthread_mutex_t **) lnxmutex;
    return pthread_cond_timedwait(maccond, macmutex, tspec);
} // mactrampoline_pthread_cond_timedwait

static int mactrampoline_pthread_cond_wait(void/*pthread_cond_t*/ *lnxcond, void/*pthread_mutex_t*/ *lnxmutex)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_cond_wait(*(pthread_cond_t **) lnxcond, *(pthread_mutex_t **) lnxmutex);
} // mactrampoline_pthread_cond_wait

static int mactrampoline_pthread_condattr_init(void/*pthread_condattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_condattr_t *macattr = (pthread_condattr_t *) malloc(sizeof (pthread_condattr_t));
    if (macattr == NULL)
        return ENOMEM;  // !!! FIXME: convert error
    const int rc = pthread_condattr_init(macattr);
    if (rc != 0)
        free(macattr);
    else
        *((void **) lnxattr) = macattr;
    return rc;
} // mactrampoline_pthread_condattr_init

static int mactrampoline_pthread_condattr_destroy(void/*pthread_condattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_condattr_t *macattr = *(pthread_condattr_t **) lnxattr;
    const int rc = pthread_condattr_destroy(macattr);
    if (rc == 0)
        free(macattr);
    return rc;
} // mactrampoline_pthread_condattr_destroy

static int mactrampoline_pthread_condattr_getpshared(const void/*pthread_condattr_t*/ *lnxattr, int *pshared)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_condattr_getpshared(*(const pthread_condattr_t **) lnxattr, pshared);
} // mactrampoline_pthread_condattr_getpshared

static int mactrampoline_pthread_condattr_setpshared(void/*pthread_condattr_t*/ *lnxattr, int pshared)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_condattr_setpshared(*(pthread_condattr_t **) lnxattr, pshared);
} // mactrampoline_pthread_condattr_setpshared

static int mactrampoline_pthread_create(pthread_t *thread, const void/*pthread_attr_t*/ *lnxattr, void *(*fn)(void*), void *fnarg)
{
    STUBBED("need to convert errors to Linux values");
    const pthread_attr_t *macattr = lnxattr ? *(const pthread_attr_t **) lnxattr : NULL;
    return pthread_create(thread, macattr, fn, fnarg);
} // mactrampoline_pthread_create

static int mactrampoline_pthread_mutex_init(void/*pthread_mutex_t*/ *lnxmutex, const void/*pthread_mutexattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_t *macmutex = (pthread_mutex_t *) malloc(sizeof (pthread_mutex_t));
    if (macmutex == NULL)
        return ENOMEM;  // !!! FIXME: convert error
    const pthread_mutexattr_t *macattr = lnxattr ? *(const pthread_mutexattr_t **) lnxattr : NULL;
    const int rc = pthread_mutex_init(macmutex, macattr);
    if (rc != 0)
        free(macmutex);
    else
        *((void **) lnxmutex) = macmutex;
    return rc;
} // mactrampoline_pthread_mutex_init

static int mactrampoline_pthread_mutex_destroy(void/*pthread_mutex_t*/ *lnxmutex)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_t *macmutex = *(pthread_mutex_t **) lnxmutex;
    const int rc = pthread_mutex_destroy(macmutex);
    if (rc == 0)
        free(macmutex);
    return rc;
} // mactrampoline_pthread_mutex_destroy

static int mactrampoline_pthread_mutex_lock(void/*pthread_mutex_t*/ *lnxmutex)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutex_lock(*(pthread_mutex_t **) lnxmutex);
} // mactrampoline_pthread_mutex_lock

#if 0 // Whoops, not in Mac OS X yet.
static int mactrampoline_pthread_mutex_timedlock(void/*pthread_mutex_t*/ *lnxmutex, const struct timespec *tspec)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutex_timedlock(*(pthread_mutex_t **) lnxmutex, tspec);
} // mactrampoline_pthread_mutex_timedlock
#endif

static int mactrampoline_pthread_mutex_trylock(void/*pthread_mutex_t*/ *lnxmutex)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutex_trylock(*(pthread_mutex_t **) lnxmutex);
} // mactrampoline_pthread_mutex_trylock

static int mactrampoline_pthread_mutex_unlock(void/*pthread_mutex_t*/ *lnxmutex)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutex_unlock(*(pthread_mutex_t **) lnxmutex);
} // mactrampoline_pthread_mutex_unlock

static int mactrampoline_pthread_mutexattr_init(void/*pthread_mutexattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutexattr_t *macattr = (pthread_mutexattr_t *) malloc(sizeof (pthread_mutexattr_t));
    if (macattr == NULL)
        return ENOMEM;  // !!! FIXME: convert error
    const int rc = pthread_mutexattr_init(macattr);
    if (rc != 0)
        free(macattr);
    else
        *((void **) lnxattr) = macattr;
    return rc;
} // mactrampoline_pthread_mutexattr_init

static int mactrampoline_pthread_mutexattr_destroy(void/*pthread_mutexattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutexattr_t *macattr = *(pthread_mutexattr_t **) lnxattr;
    const int rc = pthread_mutexattr_destroy(macattr);
    if (rc == 0)
        free(macattr);
    return rc;
} // mactrampoline_pthread_mutexattr_destroy

static int mactrampoline_pthread_mutexattr_getpshared(const void/*pthread_mutexattr_t*/ *lnxattr, int *pshared)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutexattr_getpshared(*(const pthread_mutexattr_t **) lnxattr, pshared);
} // mactrampoline_pthread_mutexattr_getpshared

static int mactrampoline_pthread_mutexattr_gettype(const void/*pthread_mutexattr_t*/ *lnxattr, int *typ)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutexattr_gettype(*(const pthread_mutexattr_t **) lnxattr, typ);
} // mactrampoline_pthread_mutexattr_gettype

static int mactrampoline_pthread_mutexattr_setpshared(void/*pthread_mutexattr_t*/ *lnxattr, int pshared)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutexattr_setpshared(*(pthread_mutexattr_t **) lnxattr, pshared);
} // mactrampoline_pthread_mutexattr_setpshared

static int mactrampoline_pthread_mutexattr_settype(void/*pthread_mutexattr_t*/ *lnxattr, int typ)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_mutexattr_settype(*(pthread_mutexattr_t **) lnxattr, typ);
} // mactrampoline_pthread_mutexattr_settype

static int mactrampoline_pthread_rwlock_init(void/*pthread_rwlock_t*/ *lnxrwlock, const void/*pthread_rwlockattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_rwlock_t *macrwlock = (pthread_rwlock_t *) malloc(sizeof (pthread_rwlock_t));
    if (macrwlock == NULL)
        return ENOMEM;  // !!! FIXME: convert error
    const pthread_rwlockattr_t *macattr = lnxattr ? *(const pthread_rwlockattr_t **) lnxattr : NULL;
    const int rc = pthread_rwlock_init(macrwlock, macattr);
    if (rc != 0)
        free(macrwlock);
    else
        *((void **) lnxrwlock) = macrwlock;
    return rc;
} // mactrampoline_pthread_rwlock_init

static int mactrampoline_pthread_rwlock_destroy(void/*pthread_rwlock_t*/ *lnxrwlock)
{
    STUBBED("need to convert errors to Linux values");
    pthread_rwlock_t *macrwlock = *(pthread_rwlock_t **) lnxrwlock;
    const int rc = pthread_rwlock_destroy(macrwlock);
    if (rc == 0)
        free(macrwlock);
    return rc;
} // mactrampoline_pthread_rwlock_destroy

static int mactrampoline_pthread_rwlock_rdlock(void/*pthread_rwlock_t*/ *lnxrwlock)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlock_rdlock(*(pthread_rwlock_t **) lnxrwlock);
} // mactrampoline_pthread_rwlock_rdlock

#if 0 // Whoops, not in Mac OS X yet.
static int mactrampoline_pthread_rwlock_timedrdlock(void/*pthread_rwlock_t*/ *lnxrwlock, const struct timespec *tspec)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlock_timedrdlock(*(pthread_rwlock_t **) lnxrwlock, tspec);
} // mactrampoline_pthread_rwlock_timedrdlock

static int mactrampoline_pthread_rwlock_timedwrlock(void/*pthread_rwlock_t*/ *lnxrwlock, const struct timespec *tspec)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlock_timedwrlock(*(pthread_rwlock_t **) lnxrwlock, tspec);
} // mactrampoline_pthread_rwlock_timedwrlock
#endif

static int mactrampoline_pthread_rwlock_tryrdlock(void/*pthread_rwlock_t*/ *lnxrwlock)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlock_tryrdlock(*(pthread_rwlock_t **) lnxrwlock);
} // mactrampoline_pthread_rwlock_tryrdlock

static int mactrampoline_pthread_rwlock_trywrlock(void/*pthread_rwlock_t*/ *lnxrwlock)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlock_trywrlock(*(pthread_rwlock_t **) lnxrwlock);
} // mactrampoline_pthread_rwlock_trywrlock

static int mactrampoline_pthread_rwlock_unlock(void/*pthread_rwlock_t*/ *lnxrwlock)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlock_unlock(*(pthread_rwlock_t **) lnxrwlock);
} // mactrampoline_pthread_rwlock_unlock

static int mactrampoline_pthread_rwlock_wrlock(void/*pthread_rwlock_t*/ *lnxrwlock)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlock_wrlock(*(pthread_rwlock_t **) lnxrwlock);
} // mactrampoline_pthread_rwlock_wrlock

static int mactrampoline_pthread_rwlockattr_init(void/*pthread_rwlockattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_rwlockattr_t *macattr = (pthread_rwlockattr_t *) malloc(sizeof (pthread_rwlockattr_t));
    if (macattr == NULL)
        return ENOMEM;  // !!! FIXME: convert error
    const int rc = pthread_rwlockattr_init(macattr);
    if (rc != 0)
        free(macattr);
    else
        *((void **) lnxattr) = macattr;
    return rc;
} // mactrampoline_pthread_rwlockattr_init

static int mactrampoline_pthread_rwlockattr_destroy(void/*pthread_rwlockattr_t*/ *lnxattr)
{
    STUBBED("need to convert errors to Linux values");
    pthread_rwlockattr_t *macattr = *(pthread_rwlockattr_t **) lnxattr;
    const int rc = pthread_rwlockattr_destroy(macattr);
    if (rc == 0)
        free(macattr);
    return rc;
} // mactrampoline_pthread_rwlockattr_destroy

static int mactrampoline_pthread_rwlockattr_getpshared(const void/*pthread_rwlockattr_t*/ *lnxattr, int *pshared)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlockattr_getpshared(*(pthread_rwlockattr_t **) lnxattr, pshared);
} // mactrampoline_pthread_rwlockattr_getpshared

static int mactrampoline_pthread_rwlockattr_setpshared(void/*pthread_rwlockattr_t*/ *lnxattr, int pshared)
{
    STUBBED("need to convert errors to Linux values");
    return pthread_rwlockattr_setpshared(*(pthread_rwlockattr_t **) lnxattr, pshared);
} // mactrampoline_pthread_rwlockattr_setpshared


// pthread_key_t is always uint32_t on Linux, it's uintptr_t on Mac OS X, so
//  we need to keep an array of them and use the Linux half as the index.
static pthread_key_t *pthread_keys = NULL;
static uint32_t num_pthread_keys = 0;
static pthread_mutex_t pthread_key_mutex;
static int mactrampoline_pthread_key_create(uint32_t/*pthread_key_t*/ *lnxpkey, void (*destructfn)(void*))
{
    // !!! FIXME: this is kinda lazy.
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_lock(&pthread_key_mutex);
    void *ptr = realloc(pthread_keys, sizeof (pthread_key_t) * (num_pthread_keys + 1));
    if (ptr == NULL)
    {
        pthread_mutex_unlock(&pthread_key_mutex);
        return ENOMEM;  // !!! FIXME: convert error
    } // if
    pthread_keys = (pthread_key_t *) ptr;
    const int rc = pthread_key_create(&pthread_keys[num_pthread_keys], destructfn);
    if (rc == 0)
        *lnxpkey = num_pthread_keys++;
    pthread_mutex_unlock(&pthread_key_mutex);
    return rc;
} // mactrampoline_pthread_getspecific

static int mactrampoline_pthread_key_delete(uint32_t/*pthread_key_t*/ lnxkey)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_lock(&pthread_key_mutex);
    int retval = 0;
    if (lnxkey >= num_pthread_keys)
        retval = EINVAL;
    else
        retval = pthread_key_delete(pthread_keys[lnxkey]);
    pthread_mutex_unlock(&pthread_key_mutex);
    return retval;
} // mactrampoline_pthread_getspecific

static int mactrampoline_pthread_setspecific(uint32_t/*pthread_key_t*/ lnxkey, const void *value)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_lock(&pthread_key_mutex);
    int retval = 0;
    if (lnxkey >= num_pthread_keys)
        retval = EINVAL;
    else
        retval = pthread_setspecific(pthread_keys[lnxkey], value);
    pthread_mutex_unlock(&pthread_key_mutex);
    return retval;
} // mactrampoline_pthread_getspecific

static void *mactrampoline_pthread_getspecific(uint32_t/*pthread_key_t*/ lnxkey)
{
    pthread_mutex_lock(&pthread_key_mutex);
    pthread_key_t *mackey = (lnxkey < num_pthread_keys) ? &pthread_keys[lnxkey] : NULL;
    void *retval = mackey ? pthread_getspecific(*mackey) : NULL;
    pthread_mutex_unlock(&pthread_key_mutex);
    return retval;
} // mactrampoline_pthread_getspecific

static int mactrampoline_pthread_once(void/*pthread_once_t*/ *_once, void (*initfn)(void))
{
    // pthread_once_t is 32-bit on all Linux arches; we can't cram the Mac
    //  version of it into that. Fortunately, it's easy enough to implement
    //  ourselves from scratch.
    // PTHREAD_ONCE_INIT on Linux is just zero, assigned to an int32_t.

    // !!! FIXME: pthread_once() has to guarantee that everything racing on
    // !!! FIXME:  the same pthread_once_t will not return until initfn()
    // !!! FIXME:  has returned, whether they were the thread to call it or not.

    volatile int32_t *once = (volatile int32_t *) _once;
    if (__sync_val_compare_and_swap(once, 0, 1) == 0)  // were we first?
        initfn();
    return 0;  // always succeed.
} // mactrampoline_pthread_once


// !!! FIXME: this should work like the native overrides, but honestly,
// !!! FIXME:  who doesn't reference glibc?
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

