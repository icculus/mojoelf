/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

// You absolutely must build this file with -mstackrealign.
//  A lot of these functions are here only as a trampoline: the ELF code calls
//  it, it aligns the stack to 16 bytes thanks to -mstackrealign, and then
//  calls the actual function in the system library.

#define _DARWIN_FEATURE_64_BIT_INODE 1
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
#include <semaphore.h>
#include <dlfcn.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <libgen.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <glob.h>

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


// by default, assume we want glX. SDL native override might want something else.
int GWantGLX = 1;

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

static int mactrampoline___vasprintf_chk(char **result, int flag, size_t buflen, const char *fmt, va_list ap)
{
    STUBBED("check flag (and stack!)");
    const int retval = vasprintf(result, fmt, ap);
    return retval;
} // mactrampoline___vsnprintf_chk

static int mactrampoline___vfprintf_chk(FILE *io, int flag, const char *fmt, va_list ap)
{
    STUBBED("check flag (and stack!)");
    const int retval = vfprintf(io, fmt, ap);
    return retval;
} // mactrampoline___vfprintf_chk

static int mactrampoline___snprintf_chk(char *buf, size_t maxbuflen, int flag, size_t buflen, const char *fmt, ...)
{
    STUBBED("check flag (and stack!)");
    STUBBED("this should abort if buflen < maxbuflen");
    va_list ap;
    va_start(ap, fmt);
    const int retval = vsnprintf(buf, buflen, fmt, ap);
    va_end(ap);
    return retval;
} // mactrampoline___snprintf_chk

static char *mactrampoline___strcat_chk(char *dst, const char *src, size_t len)
{
    STUBBED("check for overflow?");
    return strcat(dst, src);
} // mactrampoline___strcat_chk

static char *mactrampoline___strncat_chk(char *dst, const char *src, size_t len, size_t dstlen)
{
    STUBBED("check for overflow?");
    STUBBED("this should abort if len > dstlen");
    return strncat(dst, src, len);
} // mactrampoline___strncat_chk

static void *mactrampoline___memset_chk(void *dst, int ch, size_t len, size_t dstlen)
{
    STUBBED("check for overflow?");
    STUBBED("this should abort if len > dstlen");
    return memset(dst, ch, len);
} // mactrampoline___memset_chk

static void *mactrampoline___memmove_chk(void *dst, const void *src, size_t len, size_t dstlen)
{
    STUBBED("check for overflow?");
    STUBBED("this should abort if len > dstlen");
    return memmove(dst, src, len);
} // mactrampoline___memmove_chk

static void *mactrampoline___memcpy_chk(void *dst, const void *src, size_t len, size_t dstlen)
{
    STUBBED("check for overflow?");
    STUBBED("this should abort if len > dstlen");
    return memcpy(dst, src, len);
} // mactrampoline___memcpy_chk

static char *mactrampoline___strcpy_chk(char *dst, const char *src, size_t dstlen)
{
    STUBBED("check for overflow?");
    return strcpy(dst, src);
} // mactrampoline___strcpy_chk

static char *mactrampoline___strncpy_chk(char *dst, const char *src, size_t len, size_t dstlen)
{
    STUBBED("check for overflow?");
    return strncpy(dst, src, len);
} // mactrampoline___strncpy_chk

static void mactrampoline___stack_chk_fail(void)
{
    fprintf(stderr, "__stack_chk_fail!\n");
    fprintf(stderr, "Corrupted stack detected, aborting!\n\n");
    _exit(1);
} // mactrampoline___stack_chk_fail

// The Mac versions of *scanf() already work like ISO C99 (%a doesn't work like GNU).
static int mactrampoline___isoc99_fscanf(FILE *io, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int retval = vfscanf(io, fmt, ap);
    va_end(ap);
    return retval;
} // mactrampoline___isoc99_fscanf

static int mactrampoline___isoc99_scanf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int retval = vscanf(fmt, ap);
    va_end(ap);
    return retval;
} // mactrampoline___isoc99_scanf

static int mactrampoline___isoc99_sscanf(const char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int retval = vsscanf(str, fmt, ap);
    va_end(ap);
    return retval;
} // mactrampoline___isoc99_sscanf

static double mactrampoline___strtod_internal(const char *nptr, char **endptr, int group)
{
    if (group != 0)
        STUBBED("LSB expects group to be unconditionally zero");
    return strtod(nptr, endptr);
} // mactrampoline___strtod_internal

static long mactrampoline___strtol_internal(const char *nptr, char **endptr, int radix, int group)
{
    if (group != 0)
        STUBBED("LSB expects group to be unconditionally zero");
    return strtol(nptr, endptr, radix);
} // mactrampoline___strtol_internal

static unsigned long mactrampoline___strtoul_internal(const char *nptr, char **endptr, int radix, int group)
{
    if (group != 0)
        STUBBED("LSB expects group to be unconditionally zero");
    return strtoul(nptr, endptr, radix);
} // mactrampoline___strtoul_internal

static void mactrampoline___bcopy(const void *src, void *dst, size_t len)
{
    bcopy(src, dst, len);
} // mactrampoline___bcopy

static void mactrampoline___bzero(void *dst, size_t len)
{
    bzero(dst, len);
} // mactrampoline___bzero

static char *mactrampoline___xpg_basename(const char *path)
{
    STUBBED("need to decide if this every overwrites (path), and if that's okay");
    return basename((char *) path);
} // mactrampoline___xpg_basename

static int mactrampoline__IO_putc(int ch, FILE *io) { return putc(ch, io); }
static int mactrampoline__IO_getc(FILE *io) { return getc(io); }

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
static int mactrampoline_ftruncate(int fd, /*off_t*/intptr_t len)
{
    return ftruncate(fd, (off_t) len);
} // mactrampoline_ftruncate

static intptr_t/*off_t*/ mactrampoline_lseek(int fd, /*off_t*/intptr_t offset, int whence)
{
    return (intptr_t) lseek(fd, (off_t) offset, whence);
} // mactrampoline_lseek

static int64_t/*off64_t*/ mactrampoline_lseek64(int fd, int64_t/*off64_t*/ offset, int whence)
{
    return (int64_t) lseek(fd, (off_t) offset, whence);
} // mactrampoline_lseek64

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

typedef enum
{
    LINUX_MAP_SHARED=0x01,
    LINUX_MAP_PRIVATE=0x02,
    LINUX_MAP_FIXED=0x10,
    LINUX_MAP_ANON=0x20,
} LinuxMMapFlags;

static void *mactrampoline_mmap(void* addr, size_t len, int prot, int lnxflags, int fd, /*off_t*/uintptr_t offset)
{
    int macflags = 0;

    #define CVTFLAG(fl) if (lnxflags & LINUX_##fl) { macflags |= fl; lnxflags &= ~LINUX_##fl; }
    CVTFLAG(MAP_SHARED);
    CVTFLAG(MAP_PRIVATE);
    CVTFLAG(MAP_FIXED);
    CVTFLAG(MAP_ANON);
    #undef CVTFLAG

    if (lnxflags)
    {
        errno = ENOTSUP;
        return (void *) MAP_FAILED;
    } // if

    return mmap(addr, len, prot, macflags, fd, (off_t) offset);
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

static int32_t **mactrampoline___ctype_tolower_loc(void)
{
    STUBBED("this should be thread-local");
    STUBBED("...and not a race condition.");
    STUBBED("...actually, this whole thing is probably wrong anyhow.");
    static int32_t tolower_array[384];
    static int32_t *ptolower_array = tolower_array + 128;
    static int initialized = 0;
    if (!initialized)
    {
        int i;
        for (i = -128; i < 256; i++)
            tolower_array[i + 128] = (int32_t) tolower(i);
        initialized = 1;
    } // if
    return &ptolower_array;
} // mactrampoline__ctype_tolower_loc

static int32_t **mactrampoline___ctype_toupper_loc(void)
{
    STUBBED("this should be thread-local");
    STUBBED("...and not a race condition.");
    STUBBED("...actually, this whole thing is probably wrong anyhow.");
    static int32_t toupper_array[384];
    static int32_t *ptoupper_array = toupper_array + 128;
    static int initialized = 0;
    if (!initialized)
    {
        int i;
        for (i = -128; i < 256; i++)
            toupper_array[i + 128] = (int32_t) toupper(i);
        initialized = 1;
    } // if
    return &ptoupper_array;
} // mactrampoline__ctype_toupper_loc

static const uint16_t **mactrampoline___ctype_b_loc(void)
{
    STUBBED("write me");
    STUBBED("this should be thread-local, too");
//    return NULL;
    static const uint16_t barray[384];
    static const uint16_t *pbarray = barray + 128;
    return &pbarray;
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

// !!! FIXME: strictly speaking, this should be optimizable to do the right
// !!! FIXME:  thing faster under the assumption that the input is finite.
// !!! FIXME:  But this should still return correct results at a speed hit.
#define TRIG_TRAMP_FINITE(fn) \
    static double mactrampoline___##fn##_finite(double x) { return fn(x); } \
    static float mactrampoline___##fn##f_finite(float x) { return fn##f(x); } \
    static long double mactrampoline___##fn##l_finite(long double x) { return fn##l(x); }
TRIG_TRAMP_FINITE(sin)
TRIG_TRAMP_FINITE(cos)
TRIG_TRAMP_FINITE(tan)
TRIG_TRAMP_FINITE(log)
TRIG_TRAMP_FINITE(log2)
TRIG_TRAMP_FINITE(log10)
TRIG_TRAMP_FINITE(log1p)
TRIG_TRAMP_FINITE(asin)
TRIG_TRAMP_FINITE(acos)
TRIG_TRAMP_FINITE(atan)
TRIG_TRAMP_FINITE(sinh)
TRIG_TRAMP_FINITE(cosh)
TRIG_TRAMP_FINITE(tanh)
TRIG_TRAMP_FINITE(exp)
TRIG_TRAMP_FINITE(exp2)
TRIG_TRAMP_FINITE(expm1)
#undef TRIG_TRAMP_FINITE

static double mactrampoline___atan2_finite(double x, double y) { return atan2(x, y); }
static float mactrampoline___atan2f_finite(float x, float y) { return atan2f(x, y); }
static long double mactrampoline___atan2l_finite(long double x, long double y) { return atan2l(x, y); }
static double mactrampoline___pow_finite(double x, double y) { return pow(x, y); }
static float mactrampoline___powf_finite(float x, float y) { return powf(x, y); }
static long double mactrampoline___powl_finite(long double x, long double y) { return powl(x, y); }


// We only need a mutex for our global state during the
//  dlopen/dlsym/dlclone() trampolines. Initial loading happens on the main
//  (and, at the time, only) thread before handing control to the ELF code.
static pthread_mutex_t loader_mutex;

// Obviously we want to map dlopen and friends through MojoELF. We can't let
//  you talk to Mach-O binaries directly in any case, due to calling
//  convention differences.
// These aren't static, because other sources need to use them.
void *mactrampoline_dlopen(const char *soname, int flags)
{
    STUBBED("trap a few more libs like OpenAL (etc)...");
    STUBBED("flags are ignored");
    pthread_mutex_lock(&loader_mutex);
    // !!! FIXME: we need the executable's rpath and runpath settings.
    MOJOELF_dlerror();  // clear any existing errors.
    // !!! FIXME: there are paths to failure that won't go
    // !!! FIXME:  through MojoELF, and thus won't set the error.
    void *retval = mojoelf_callbacks.loader(soname, NULL, NULL);
    pthread_mutex_unlock(&loader_mutex);
    return retval;
} // mactrampoline_dlopen

void *mactrampoline_dlsym(void *lib, const char *sym)
{
    // !!! FIXME: lib==NULL means something different.
    pthread_mutex_lock(&loader_mutex);
    MOJOELF_dlerror();  // clear any existing errors.
    // !!! FIXME: there are paths to failure that won't go
    // !!! FIXME:  through MojoELF, and thus won't set the error.
    void *retval = mojoelf_callbacks.resolver(lib, sym);
    pthread_mutex_unlock(&loader_mutex);
    return retval;
} // mactrampoline_dlsym

int mactrampoline_dlclose(void *lib)
{
    pthread_mutex_lock(&loader_mutex);
    MOJOELF_dlerror();  // clear any existing errors.
    mojoelf_callbacks.unloader(lib);
    pthread_mutex_unlock(&loader_mutex);
    return 0;
} // mactrampoline_dlclose

char *mactrampoline_dlerror(void)
{
    STUBBED("This should make a temp copy if this is really non-const");
    return (char *) MOJOELF_dlerror();
} // mactrampoline_dlerror


static int mactrampoline_ioctl(int fd, int req, ...)
{
    STUBBED("this is going to need a bunch of complexity");
    fprintf(stderr, "WARNING: unhandled ioctl(%d, %d, ...) called!\n", fd, req);
    errno = ENOTSUP;
    return -1;
} // mactrampoline_ioctl

static int mactrampoline_fcntl(int fd, int cmd, ...)
{
    STUBBED("this is going to need a bunch of complexity");
    fprintf(stderr, "WARNING: unhandled fcntl(%d, %d, ...) called!\n", fd, cmd);
    errno = ENOTSUP;
    return -1;
} // mactrampoline_fcntl

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

static void mactrampoline_syslog(int priority, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(priority, fmt, ap);
    va_end(ap);
} // mactrampoline_syslog

static int mactrampoline_mkstemp64(char *templt)
{
    return mkstemp(templt);  // we're already 64-bit, just pass it through.
} // mactrampoline_mkstemp64

typedef struct
{
    char *ptr;
    int len;
    int pos;
} MtabIo;

static int mtab_read(void *opaque, char *buf, int len)
{
    MtabIo *iodata = (MtabIo *) opaque;
    const int avail = iodata->len - iodata->pos;
    if (len > avail)
        len = avail;
    if (len > 0)
    {
        memcpy(buf, iodata->ptr + iodata->pos, len);
        iodata->pos += len;
    } // if
    return len;
} // mtab_read

static int mtab_close(void *opaque)
{
    MtabIo *iodata = (MtabIo *) opaque;
    free(iodata->ptr);
    free(iodata);
    return 0;
} // mtab_close


static FILE *mactrampoline_setmntent(const char *fname, const char *mode)
{
    if (strcmp(fname, "/etc/mtab") != 0)
        return fopen(fname, mode);

    char *mtab = NULL;
    int mtablen = 0;
    int i = 0;
    struct statfs *mntbufp = NULL;
    const int mounts = getmntinfo(&mntbufp, MNT_WAIT);
    for (i = 0; i < mounts; i++)
    {
        char opts[256];
        char buf[1024];

        strcpy(opts, (mntbufp->f_flags & MNT_RDONLY) ? "ro" : "rw");
        #define CVTFLAG(fl,str) { \
            if (mntbufp->f_flags & fl) { \
                strcat(opts, "," str); \
            } \
        }
        CVTFLAG(MNT_SYNCHRONOUS, "sync");
        CVTFLAG(MNT_ASYNC, "async");
        CVTFLAG(MNT_NOEXEC, "noexec");
        CVTFLAG(MNT_NOSUID, "nosuid");
        CVTFLAG(MNT_NODEV, "nodev");
        #undef CVTFLAG

        snprintf(buf, sizeof (buf), "%s %s %s %s 0 0\n",
                 mntbufp->f_mntfromname, mntbufp->f_mntonname,
                 mntbufp->f_fstypename, opts);
        const size_t len = strlen(buf);
        void *ptr = realloc(mtab, mtablen + len + 1);
        if (ptr == NULL)
        {
            free(mtab);
            errno = ENOMEM;
            return NULL;
        } // if
        mtab = (char *) ptr;
        strcpy(mtab + mtablen, buf);
        mtablen += len;
        mntbufp++;
    } // for

    MtabIo *iodata = (MtabIo *) malloc(sizeof (MtabIo));
    if (iodata == NULL)
    {
        free(mtab);
        return NULL;
    } // if

    iodata->ptr = mtab;
    iodata->len = mtablen;
    iodata->pos = 0;

    // !!! FIXME: writing
    FILE *retval = funopen(iodata, mtab_read, NULL, NULL, mtab_close);
    if (retval == NULL)
    {
        free(mtab);
        free(iodata);
    } // if
    return retval;
} // mactrampoline_setmntent

typedef struct
{
    char *mnt_fsname;
    char *mnt_dir;
    char *mnt_type;
    char *mnt_opts;
    int   mnt_freq;
    int   mnt_passno;
} LinuxMntEnt;

static LinuxMntEnt *mactrampoline_getmntent(FILE *io)
{
    static LinuxMntEnt retval;
    static char buf[512];
    if (!fgets(buf, sizeof (buf), io))
        return NULL;

    char *tok = strtok(buf, " ");
    if (!tok) return NULL;
    retval.mnt_fsname = tok;

    tok = strtok(NULL, " ");
    if (!tok) return NULL;
    retval.mnt_dir = tok;

    tok = strtok(NULL, " ");
    if (!tok) return NULL;
    retval.mnt_type = tok;

    tok = strtok(NULL, " ");
    if (!tok) return NULL;
    retval.mnt_opts = tok;

    tok = strtok(NULL, " ");
    if (!tok) return NULL;
    retval.mnt_freq = atoi(tok);   // !!! FIXME: don't use atoi.

    tok = strtok(NULL, " ");
    if (!tok) return NULL;
    retval.mnt_passno = atoi(tok);   // !!! FIXME: don't use atoi.

    return &retval;
} // mactrampoline_getmntent

//static int mactrampoline_addmntent(FILE *io, const struct LinuxMntEnt *b)

static int mactrampoline_endmntent(FILE *io)
{
    return (io == NULL) ? 0 : fclose(io);
} // mactrampoline_endmntent

//static char* mactrampoline_hasmntopt(const struct mntent *a, const char *b)

#define LINUX_UTSNAME_STRING_LENGTH 65
typedef struct
{
    char sysname[LINUX_UTSNAME_STRING_LENGTH];
    char nodename[LINUX_UTSNAME_STRING_LENGTH];
    char release[LINUX_UTSNAME_STRING_LENGTH];
    char version[LINUX_UTSNAME_STRING_LENGTH];
    char machine[LINUX_UTSNAME_STRING_LENGTH];
    char domainname[LINUX_UTSNAME_STRING_LENGTH];
} LinuxUtsName;

static int mactrampoline_uname(LinuxUtsName *lnxname)
{
    struct utsname macname;
    if (uname(&macname) == -1)
        return -1;
    else if (gethostname(lnxname->domainname, sizeof (lnxname->domainname)) == -1)
        return -1;

    #define CPYUTSSTR(x) snprintf(lnxname->x, sizeof (lnxname->x), "%s", macname.x)
    CPYUTSSTR(sysname);
    CPYUTSSTR(nodename);
    CPYUTSSTR(release);
    CPYUTSSTR(version);
    CPYUTSSTR(machine);
    #undef CPYUTSSTR

    return 0;
} // mactrampoline_uname

typedef enum
{
    LINUX_GLOB_ERR         =(1 << 0),
    LINUX_GLOB_MARK        =(1 << 1),
    LINUX_GLOB_NOSORT      =(1 << 2),
    LINUX_GLOB_DOOFFS      =(1 << 3),
    LINUX_GLOB_NOCHECK     =(1 << 4),
    LINUX_GLOB_APPEND      =(1 << 5),
    LINUX_GLOB_NOESCAPE    =(1 << 6),
    LINUX_GLOB_PERIOD      =(1 << 7),
    LINUX_GLOB_MAGCHAR     =(1 << 8),
    LINUX_GLOB_ALTDIRFUNC  =(1 << 9),
    LINUX_GLOB_BRACE       =(1 << 10),
    LINUX_GLOB_NOMAGIC     =(1 << 11),
    LINUX_GLOB_TILDE       =(1 << 12),
    LINUX_GLOB_ONLYDIR     =(1 << 13),
    LINUX_GLOB_TILDE_CHECK =(1 << 14),
} LinuxGlobFlags;

typedef enum
{
    LINUX_GLOB_NOSPACE=1,
    LINUX_GLOB_ABORTED=2,
    LINUX_GLOB_NOMATCH=3,
    LINUX_GLOB_NOSYS=4,
} LinuxGlobErrors;

// This matches the size of Mac's glob_t (once you drop two Mac-specific fields),
//  it just needs to be reorganized a little, and flags need to be remapped.
//  Note that the 64-bit version of this matches, too, but some of these
//  void pointers point to 64-bit structures. The Mac side is always 64-bit.
typedef struct
{
    size_t gl_pathc;
    char **gl_pathv;
    size_t gl_offs;
    int gl_flags;
	void (*gl_closedir)(void *);
	void *(*gl_readdir)(void *);
	void *(*gl_opendir)(const char *);
	int (*gl_lstat)(const char *, void *);
	int (*gl_stat)(const char *, void *);
} LinuxGlobT;

static int linux_glob_t_to_mac(glob_t *macglob, const LinuxGlobT *lnxglob)
{
    int macflags = 0;
    int lnxflags = lnxglob->gl_flags;

    if (lnxflags & LINUX_GLOB_ALTDIRFUNC)
        STUBBED("we have to bridge the GLOB_ALTDIRFUNC callbacks");

    // Linux flags aren't the same values, so map them.
    #define MAPFLAG(fl) if (lnxflags & LINUX_##fl) { lnxflags &= ~LINUX_##fl; macflags |= fl; }
    MAPFLAG(GLOB_ERR);
    MAPFLAG(GLOB_MARK);
    MAPFLAG(GLOB_NOSORT);
    MAPFLAG(GLOB_DOOFFS);
    MAPFLAG(GLOB_NOCHECK);
    MAPFLAG(GLOB_APPEND);
    MAPFLAG(GLOB_NOESCAPE);
    //MAPFLAG(GLOB_PERIOD);
    MAPFLAG(GLOB_MAGCHAR);
    //MAPFLAG(GLOB_ALTDIRFUNC);
    MAPFLAG(GLOB_BRACE);
    MAPFLAG(GLOB_NOMAGIC);
    MAPFLAG(GLOB_TILDE);
    //MAPFLAG(GLOB_ONLYDIR);
    //MAPFLAG(GLOB_TILDE_CHECK);
    #undef MAPFLAG

    if (lnxflags)
        return 0;

    macglob->gl_pathc = lnxglob->gl_pathc;
    macglob->gl_matchc = 0;
    macglob->gl_offs = lnxglob->gl_offs;
    macglob->gl_errfunc = NULL;
	macglob->gl_flags = macflags;
	macglob->gl_pathv = lnxglob->gl_pathv;
	macglob->gl_closedir = lnxglob->gl_closedir;
	macglob->gl_readdir = (struct dirent *(*)(void*)) lnxglob->gl_readdir;
	macglob->gl_opendir = lnxglob->gl_opendir;
	macglob->gl_lstat = (int (*)(const char *, struct stat *)) lnxglob->gl_lstat;
	macglob->gl_stat = (int (*)(const char *, struct stat *)) lnxglob->gl_stat;

    return 1;
} // linux_glob_t_to_mac

static int mac_glob_t_to_linux(LinuxGlobT *lnxglob, const glob_t *macglob)
{
    int lnxflags = 0;
    int macflags = macglob->gl_flags;

    // Linux flags aren't the same values, so map them.
    #define MAPFLAG(fl) if (macflags & fl) { macflags &= ~fl; lnxflags |= LINUX_##fl; }
    MAPFLAG(GLOB_ERR);
    MAPFLAG(GLOB_MARK);
    MAPFLAG(GLOB_NOSORT);
    MAPFLAG(GLOB_DOOFFS);
    MAPFLAG(GLOB_NOCHECK);
    MAPFLAG(GLOB_APPEND);
    MAPFLAG(GLOB_NOESCAPE);
    MAPFLAG(GLOB_MAGCHAR);
    MAPFLAG(GLOB_ALTDIRFUNC);
    MAPFLAG(GLOB_BRACE);
    MAPFLAG(GLOB_NOMAGIC);
    MAPFLAG(GLOB_TILDE);
    #undef MAPFLAG

    if (macflags)
        return 0;

    lnxglob->gl_pathc = macglob->gl_pathc;
    lnxglob->gl_offs = macglob->gl_offs;
	lnxglob->gl_flags = macglob->gl_flags;
	lnxglob->gl_pathv = macglob->gl_pathv;
	lnxglob->gl_closedir = macglob->gl_closedir;
	lnxglob->gl_readdir = (void *(*)(void*)) macglob->gl_readdir;
	lnxglob->gl_opendir = macglob->gl_opendir;
	lnxglob->gl_lstat = (int (*)(const char *, void *)) macglob->gl_lstat;
	lnxglob->gl_stat = (int (*)(const char *, void *)) macglob->gl_stat;

    return 1;
} // mac_glob_t_to_linux

static int mactrampoline_glob(const char *pattern, int lnxflags, int (*errfn)(const char *, int), LinuxGlobT *lnxglob)
{
    glob_t macglob;

    lnxglob->gl_flags = lnxflags;
    if (!linux_glob_t_to_mac(&macglob, lnxglob))
        return LINUX_GLOB_NOSYS;   // uhoh, something we didn't handle!

    STUBBED("errfn needs to have errno values bridged");
    const int rc = glob(pattern, macglob.gl_flags, errfn, &macglob);
    mac_glob_t_to_linux(lnxglob, &macglob);

    if (rc)
    {
        switch (rc)
        {
            #define MAPERR(x) case LINUX_##x: return x
            MAPERR(GLOB_NOSPACE);
            MAPERR(GLOB_ABORTED);
            MAPERR(GLOB_NOMATCH);
            #undef MAPERR
        } // switch
    } // if

    return 0;  // success!
} // mactrampoline_glob

static void mactrampoline_globfree(LinuxGlobT *lnxglob)
{
    glob_t macglob;
    linux_glob_t_to_mac(&macglob, lnxglob);
    globfree(&macglob);
} // mactrampoline_globfree

static int mactrampoline_sigaction(int sig, const void/*struct sigaction*/ *lnxact, void/*struct sigaction*/ *lnxoact)
{
    STUBBED("write me");
    errno = ENOTSUP;
    return -1;
} // mactrampoline_sigaction

static int mactrampoline_fcvt_r(double number, int ndigits, int *decpt, int *sign, char *buf, size_t len)
{
    STUBBED("this is wrong: not reentrant, wrong retval");
    char *str = fcvt(number, ndigits, decpt, sign);
    snprintf(buf, len, "%s", str);
    return 0;
} // mactrampoline_fcvt_r

static int mactrampoline_getrlimit(int resource, struct rlimit *limit)
{
    STUBBED("write me");
    errno = ENOTSUP;
    return -1;
} // mactrampoline_getrlimit

static int mactrampoline_setrlimit(int resource, const struct rlimit *limit)
{
    STUBBED("write me");
    errno = ENOTSUP;
    return -1;
} // mactrampoline_setrlimit

static int mactrampoline_getrlimit64(int resource, struct rlimit *limit)
{
    STUBBED("write me");
    errno = ENOTSUP;
    return -1;
} // mactrampoline_getrlimit64

static int mactrampoline_setrlimit64(int resource, const struct rlimit *limit)
{
    STUBBED("write me");
    errno = ENOTSUP;
    return -1;
} // mactrampoline_setrlimit64

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

static void mactrampoline__longjmp(jmp_buf env, int val)
{
    longjmp(env, val);
} // mactrampoline__longjmp

#define LINUX_SEM_FAILED ((sem_t *)0)

static sem_t *mactrampoline_sem_open(const char *name, int lnxflags, ...)
{
    mode_t mode = 0;
    unsigned int initialval = 0;
    int macflags = 0;

    // Linux flags aren't the same values, so map them.
    // O_RDONLY is zero in both of them, though.
    #define MAPFLAG(fl) if (lnxflags & LINUX_##fl) { lnxflags &= ~LINUX_##fl; macflags |= fl; }
    MAPFLAG(O_CREAT);
    MAPFLAG(O_EXCL);
    #undef MAPFLAG

    if (lnxflags)  // uhoh, something we didn't handle!
    {
        errno = ENOTSUP;
        return LINUX_SEM_FAILED;
    } // if

    // Have to use varargs if O_CREAT, grumble grumble.
    if (macflags & O_CREAT)
    {
        va_list ap;
        va_start(ap, lnxflags);
        mode = (mode_t) va_arg(ap, uint32_t);  // mode_t is 4 bytes on Linux, 2 on Mac.
        initialval = va_arg(ap, unsigned int);
        va_end(ap);
    } // if

    sem_t *retval = sem_open(name, macflags, mode, initialval);
    return (retval == SEM_FAILED) ? LINUX_SEM_FAILED : retval;
} // mactrampoline_sem_open

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
    pthread_mutex_t *macmutex = *(pthread_mutex_t **) lnxmutex;
    return macmutex ? pthread_mutex_lock(macmutex) : -EINVAL;
} // mactrampoline_pthread_mutex_lock

#if 0 // Whoops, not in Mac OS X yet.
static int mactrampoline_pthread_mutex_timedlock(void/*pthread_mutex_t*/ *lnxmutex, const struct timespec *tspec)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_t *macmutex = *(pthread_mutex_t **) lnxmutex;
    return macmutex ? pthread_mutex_timedlock(macmutex, tspec) : -EINVAL;
} // mactrampoline_pthread_mutex_timedlock
#endif

static int mactrampoline_pthread_mutex_trylock(void/*pthread_mutex_t*/ *lnxmutex)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_t *macmutex = *(pthread_mutex_t **) lnxmutex;
    return macmutex ? pthread_mutex_trylock(macmutex) : -EINVAL;
} // mactrampoline_pthread_mutex_trylock

static int mactrampoline_pthread_mutex_unlock(void/*pthread_mutex_t*/ *lnxmutex)
{
    STUBBED("need to convert errors to Linux values");
    pthread_mutex_t *macmutex = *(pthread_mutex_t **) lnxmutex;
    return macmutex ? pthread_mutex_unlock(macmutex) : -EINVAL;
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

static int mactrampoline_pthread_mutexattr_setkind_np(void/*pthread_mutexattr_t*/ *lnxattr, int typ)
{
    return mactrampoline_pthread_mutexattr_settype(lnxattr, typ);
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



// The things we (probably) care about are AF_UNIX, AF_INET, and AF_INET6.
//  Most of the other ones don't match up, but they aren't things we care about.
//  AF_UNIX and AF_INET use the same values as Mac OS X. AF_INET6 doesn't.
#define LINUX_AF_INET6 10
#define LINUX_PF_INET6 LINUX_AF_INET6

static size_t get_sockaddr_len(const int family)
{
    switch (family)
    {
        case AF_UNIX: return sizeof (struct sockaddr_un);
        case AF_INET: return sizeof (struct sockaddr_in);
        case AF_INET6: return sizeof (struct sockaddr_in6);
    } // switch
    return 0;
} // get_sockaddr_len

static inline int supported_socket_family(const int family)
{
    return (get_sockaddr_len(family) != 0);
} // supported_socket_family

static int linux_sockaddr_to_mac(void *macaddr, const void *lnxaddr)
{
    uint16_t family = *((uint16_t *) lnxaddr);
    if (family == LINUX_AF_INET6)
        family = AF_INET6;

    const size_t structlen = get_sockaddr_len(family);
    if (!structlen)
    {
        errno = ENOTSUP;
        return 0;
    } // if

    // the things we currently care about all match sizes.
    memcpy(macaddr, lnxaddr, structlen);

    // Linux uses 16 bits for the family, Mac uses 8 for the struct size, 8 for the family.
    uint8_t *ptr = (uint8_t *) macaddr;
    *(ptr++) = (uint8_t) structlen;
    *(ptr++) = (uint8_t) family;

    return 1;
} // linux_sockaddr_to_mac

static int mac_sockaddr_to_linux(void *lnxaddr, const void *macaddr)
{
    // The things we (probably) care about are AF_UNIX, AF_INET, and AF_INET6.
    //  Most of the other ones don't match up, but they aren't things we care about.
    const uint8_t structlen = ((uint8_t *) macaddr)[0];
    uint8_t family = ((uint8_t *) macaddr)[1];

    if (family == AF_INET6)
        family = LINUX_AF_INET6;

    if (!supported_socket_family(family))
    {
        errno = ENOTSUP;
        return 0;
    } // if

    // the things we currently care about all match sizes.
    memcpy(lnxaddr, macaddr, (size_t) structlen);

    // Linux uses 16 bits for the family, Mac uses 8 for the struct size, 8 for the family.
    uint16_t *ptr = (uint16_t *) lnxaddr;
    *ptr = (uint16_t) family;

    return 1;
} // mac_sockaddr_to_linux

static int mactrampoline_accept(int fd, void/*struct sockaddr*/ *addr, socklen_t *len)
{
    struct sockaddr_storage macaddr;
    const int rc = accept(fd, (struct sockaddr *) &macaddr, len);
    if (rc == -1)
        return -1;

    if (!mac_sockaddr_to_linux(addr, &macaddr))
    {
        close(rc);  // oh well.
        return -1;
    } // if

    return rc;
} // mactrampoline_accept

static int mactrampoline_bind(int fd, const void/*struct sockaddr*/ *addr, socklen_t len)
{
    struct sockaddr_storage macaddr;
    if (!linux_sockaddr_to_mac(&macaddr, addr))
        return -1;
    return bind(fd, (struct sockaddr *) &macaddr, len);
} // mactrampoline_bind

static int mactrampoline_connect(int fd, const void/*struct sockaddr*/ *addr, socklen_t len)
{
    struct sockaddr_storage macaddr;
    if (!linux_sockaddr_to_mac(&macaddr, addr))
        return -1;
    return connect(fd, (struct sockaddr *) &macaddr, len);
} // mactrampoline_connect

static int mactrampoline_getpeername(int fd, void/*struct sockaddr*/ *addr, socklen_t *len)
{
    struct sockaddr_storage macaddr;
    const int rc = getpeername(fd, (struct sockaddr *) &macaddr, len);
    if (rc == -1)
        return -1;
    else if (!mac_sockaddr_to_linux(addr, &macaddr))
        return -1;
    return 0;
} // mactrampoline_getpeername

static int mactrampoline_getsockname(int fd, void/*struct sockaddr*/ *addr, socklen_t *len)
{
    struct sockaddr_storage macaddr;
    const int rc = getsockname(fd, (struct sockaddr *) &macaddr, len);
    if (rc == -1)
        return -1;
    else if (!mac_sockaddr_to_linux(addr, &macaddr))
        return -1;
    return 0;
} // mactrampoline_getsockname

static const char *mactrampoline_inet_ntop(int family, const void *src, char *dst, socklen_t len)
{
    if (family == LINUX_AF_INET6)
        family = AF_INET6;
    if (!supported_socket_family(family))
    {
        errno = EAFNOSUPPORT;
        return NULL;
    } // if
    return inet_ntop(family, src, dst, len);
} // mactrampoline_inet_ntop

static int mactrampoline_inet_pton(int family, const char *src, void *dst)
{
    if (family == LINUX_AF_INET6)
        family = AF_INET6;
    if (!supported_socket_family(family))
    {
        errno = EAFNOSUPPORT;
        return -1;
    } // if
    return inet_pton(family, src, dst);
} // mactrampoline_inet_pton

static void mactrampoline_freeaddrinfo(struct addrinfo *info)
{
    // Linux and Mac addrinfo mostly line up here; two fields are swapped, and PF_INET6 is a different value.
    char *ai_canonname = (char *) info->ai_addr;
    struct sockaddr *ai_addr = (struct sockaddr *) info->ai_canonname;
    info->ai_canonname = ai_canonname;
    info->ai_addr = ai_addr;
    if (info->ai_family == LINUX_PF_INET6)
        info->ai_family = PF_INET6;
    freeaddrinfo(info);
} // mactrampoline_freeaddrinfo

typedef enum
{
    LINUX_EAI_BADFLAGS=-1,
    LINUX_EAI_NONAME=-2,
    LINUX_EAI_AGAIN=-3,
    LINUX_EAI_FAIL=-4,
    LINUX_EAI_FAMILY=-6,
    LINUX_EAI_SOCKTYPE=-7,
    LINUX_EAI_SERVICE=-8,
    LINUX_EAI_MEMORY=-10,
    LINUX_EAI_SYSTEM=-11,
    LINUX_EAI_OVERFLOW=-12,
} LinuxAddrInfoErrors;

static const char *mactrampoline_gai_strerror(int err)
{
    switch ((LinuxAddrInfoErrors) err)
    {
        case LINUX_EAI_BADFLAGS: return "invalid value for ai_flags";
        case LINUX_EAI_NONAME: return "hostname or servname not provided, or not known";
        case LINUX_EAI_AGAIN: return "temporary failure in name resolution";
        case LINUX_EAI_FAIL: return "non-recoverable failure in name resolution";
        case LINUX_EAI_FAMILY: return "ai_family not supported";
        case LINUX_EAI_SOCKTYPE: return "ai_socktype not supported";
        case LINUX_EAI_SERVICE: return "servname not supported for ai_socktype";
        case LINUX_EAI_MEMORY: return " memory allocation failure";
        case LINUX_EAI_SYSTEM: return "system error returned in errno";
        case LINUX_EAI_OVERFLOW: return "argument buffer overflow";
    } // switch

    return "unknown error";
} // mactrampoline_gai_strerror

static int mac_addrinfoerror_to_linux(const int err)
{
    switch (err)
    {
        case 0: return 0;
        #define CVTERRCODE(e) case e: return LINUX_##e
        CVTERRCODE(EAI_BADFLAGS);
        CVTERRCODE(EAI_NONAME);
        CVTERRCODE(EAI_AGAIN);
        CVTERRCODE(EAI_FAIL);
        CVTERRCODE(EAI_FAMILY);
        CVTERRCODE(EAI_SOCKTYPE);
        CVTERRCODE(EAI_SERVICE);
        CVTERRCODE(EAI_MEMORY);
        CVTERRCODE(EAI_SYSTEM);
        CVTERRCODE(EAI_OVERFLOW);
        #undef CVTERRCODE
    } // switch
    return LINUX_EAI_FAIL;  // oh well.
} // mac_addrinfoerror_to_linux

static int mactrampoline_getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
    // getaddrinfo on Linux and Mac currently only accepts PF_INET, PF_INET6, and PF_UNSPEC.
    // Linux and Mac addrinfo mostly line up here; two fields are swapped, and PF_INET6 is a different value.
    struct addrinfo info;
    memcpy(&info, hints, sizeof (info));
    info.ai_canonname = (char *) hints->ai_addr;
    info.ai_addr = (struct sockaddr *) hints->ai_canonname;
    if (info.ai_family == LINUX_PF_INET6)
        info.ai_family = PF_INET6;

    const int rc = getaddrinfo(hostname, servname, &info, res);

    // swap the result fields one more time...
    if ((res != NULL) && (*res != NULL))
    {
        struct addrinfo *pinfo = *res;
        char *ai_canonname = (char *) pinfo->ai_addr;
        struct sockaddr *ai_addr = (struct sockaddr *) pinfo->ai_canonname;
        pinfo->ai_canonname = ai_canonname;
        pinfo->ai_addr = ai_addr;
        if (pinfo->ai_family == PF_INET6)
            pinfo->ai_family = LINUX_PF_INET6;
    } // if

    return mac_addrinfoerror_to_linux(rc);
} // mactrampoline_getaddrinfo

typedef enum
{
    LINUX_NI_NUMERICHOST=1,
    LINUX_NI_NUMERICSERV=2,
    LINUX_NI_NOFQDN=4,
    LINUX_NI_NAMEREQD=8,
    LINUX_NI_DGRAM=16
} LinuxGetNameInfoFlags;

static int mactrampoline_getnameinfo(const void/*struct sockaddr*/ *addr, socklen_t addrlen, char *host, socklen_t hostlen, char *service, socklen_t servlen, int lnxflags)
{
    struct sockaddr_storage macaddr;
    int macflags = 0;

    #define CVTFLAG(fl) if (lnxflags & LINUX_##fl) { macflags |= fl; lnxflags &= ~LINUX_##fl; }
    CVTFLAG(NI_NUMERICHOST);
    CVTFLAG(NI_NUMERICSERV);
    CVTFLAG(NI_NOFQDN);
    CVTFLAG(NI_NAMEREQD);
    CVTFLAG(NI_DGRAM);
    #undef CVTFLAG

    if (lnxflags != 0)
    {
        STUBBED("unsupported flag");
        errno = EINVAL;
        return -1;  // flag we don't handle.
    } // if

    if (!linux_sockaddr_to_mac(&macaddr, addr))
        return -1;

    const int rc = getnameinfo((struct sockaddr *) &macaddr, addrlen, host, hostlen, service, servlen, macflags);
    return mac_addrinfoerror_to_linux(rc);
} // mactrampoline_getnameinfo


#define LINUX_SOL_SOCKET 1
typedef enum
{
    LINUX_SO_DEBUG=1,
    LINUX_SO_REUSEADDR=2,
    LINUX_SO_TYPE=3,
    LINUX_SO_ERROR=4,
    LINUX_SO_DONTROUTE=5,
    LINUX_SO_BROADCAST=6,
    LINUX_SO_SNDBUF=7,
    LINUX_SO_RCVBUF=8,
    LINUX_SO_KEEPALIVE=9,
    LINUX_SO_OOBINLINE=10,
    LINUX_SO_LINGER=13,
    //LINUX_SO_BSDCOMPAT=14,
    LINUX_SO_RCVLOWAT=18,
    LINUX_SO_SNDLOWAT=19,
    LINUX_SO_RCVTIMEO=20,
    LINUX_SO_SNDTIMEO=21,
} LinuxSocketOptions;

static int linux_sockopt_to_mac(const int lnxopt)
{
    switch ((LinuxSocketOptions) lnxopt)
    {
        #define CVTSOCKOPT(opt) case LINUX_##opt: return opt
        CVTSOCKOPT(SO_DEBUG);
        CVTSOCKOPT(SO_REUSEADDR);
        CVTSOCKOPT(SO_TYPE);
        CVTSOCKOPT(SO_ERROR);
        CVTSOCKOPT(SO_DONTROUTE);
        CVTSOCKOPT(SO_BROADCAST);
        CVTSOCKOPT(SO_SNDBUF);
        CVTSOCKOPT(SO_RCVBUF);
        CVTSOCKOPT(SO_KEEPALIVE);
        CVTSOCKOPT(SO_OOBINLINE);
        CVTSOCKOPT(SO_LINGER);
        CVTSOCKOPT(SO_RCVLOWAT);
        CVTSOCKOPT(SO_SNDLOWAT);
        CVTSOCKOPT(SO_RCVTIMEO);
        CVTSOCKOPT(SO_SNDTIMEO);
        #undef CVTSOCKOPT
    } // switch

    return -1;
} // linux_sockopt_to_mac

static int mactrampoline_setsockopt(int fd, int level, int option, const void *value, socklen_t len)
{
    if (level == LINUX_SOL_SOCKET)
        level = SOL_SOCKET;
    option = linux_sockopt_to_mac(option);
    if (option == -1)
    {
        errno = ENOTSUP;
        return -1;
    } // if
    return setsockopt(fd, level, option, value, len);
} // mactrampoline_setsockopt

static int mactrampoline_getsockopt(int fd, int level, int option, void *value, socklen_t *len)
{
    if (level == LINUX_SOL_SOCKET)
        level = SOL_SOCKET;
    option = linux_sockopt_to_mac(option);
    if (option == -1)
    {
        errno = ENOTSUP;
        return -1;
    } // if
    return getsockopt(fd, level, option, value, len);
} // mactrampoline_getsockopt


typedef enum
{
    LINUX_MSG_OOB=0x01,
    LINUX_MSG_PEEK=0x02,
    LINUX_MSG_DONTROUTE=0x04,
    LINUX_MSG_CTRUNC=0x08,
    LINUX_MSG_TRUNC=0x20,
    LINUX_MSG_DONTWAIT=0x40,
    LINUX_MSG_EOR=0x80,
    LINUX_MSG_WAITALL=0x100,
} LinuxSendRecvFlags;

static int linux_sendrecvflags_to_mac(int lnxflags)
{
    int macflags = 0;

    #define CVTFLAG(fl) if (lnxflags & LINUX_##fl) { macflags |= fl; lnxflags &= ~LINUX_##fl; }
    CVTFLAG(MSG_OOB);
    CVTFLAG(MSG_PEEK);
    CVTFLAG(MSG_DONTROUTE);
    CVTFLAG(MSG_CTRUNC);
    CVTFLAG(MSG_TRUNC);
    CVTFLAG(MSG_DONTWAIT);
    CVTFLAG(MSG_EOR);
    CVTFLAG(MSG_WAITALL);
    #undef CVTFLAG

    if (lnxflags != 0)
    {
        STUBBED("unsupported flag");
        errno = EINVAL;
        return -1;
    } // if

    assert(macflags != -1);
    return macflags;
} // linux_sendrecvflags_to_mac

static ssize_t mactrampoline_recv(int fd, void *buf, size_t buflen, int lnxflags)
{
    const int macflags = linux_sendrecvflags_to_mac(lnxflags);
    return (macflags == -1) ? -1 : recv(fd, buf, buflen, macflags);
} // mactrampoline_recv

static ssize_t mactrampoline_recvfrom(int fd, void *buf, size_t buflen, int lnxflags, void/*struct sockaddr*/ *addr, socklen_t *addrlen)
{
    struct sockaddr_storage macaddr;
    const int macflags = linux_sendrecvflags_to_mac(lnxflags);
    if (macflags == -1)
        return -1;
    const int rc = recvfrom(fd, buf, buflen, macflags, (struct sockaddr *) &macaddr, addrlen);
    if (rc != -1)
    {
        if (!mac_sockaddr_to_linux(addr, &macaddr))
            return -1;
    } // if
    return rc;
} // mactrampoline_recvfrom

static ssize_t mactrampoline_send(int fd, const void *buf, size_t buflen, int lnxflags)
{
    const int macflags = linux_sendrecvflags_to_mac(lnxflags);
    return (macflags == -1) ? -1 : send(fd, buf, buflen, macflags);
} // mactrampoline_send

static ssize_t mactrampoline_sendto(int fd, const void *buf, size_t buflen, int lnxflags, const void/*struct sockaddr*/ *addr, socklen_t addrlen)
{
    struct sockaddr_storage macaddr;
    const int macflags = linux_sendrecvflags_to_mac(lnxflags);
    if (macflags == -1)
        return -1;
    else if (!linux_sockaddr_to_mac(&macaddr, addr))
        return -1;
    return sendto(fd, buf, buflen, macflags, (struct sockaddr *) &macaddr, addrlen);
} // mactrampoline_sendto


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

