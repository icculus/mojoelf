#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "mojoelf.h"
#include "hashtable.h"

// !!! FIXME: cut and pasted from mactrampolines.c ...
#define STUBBED(x) do { \
    static int seen_this = 0; \
    if (!seen_this) { \
        seen_this = 1; \
        fprintf(stderr, "STUBBED: %s at %s (%s:%d)\n", x, __FUNCTION__, __FILE__, __LINE__); \
    } \
} while (0)

static int run_with_missing_symbols = 0;
static int report_missing_symbols = 0;
static int symbols_missing = 0;
static int dependencies_missing = 0;

char *program_invocation_name = NULL;
const char *ld_library_path = NULL;

static HashTable *resolver_symbols = NULL;
static HashTable *loaded_ELFs = NULL;

typedef struct
{
    char *soname;
    void *handle;
    int refcount;
} LoadedLibrary;


static void *mojoelf_loader(const char *soname, const char *rpath, const char *runpath);
static void *mojoelf_resolver(void *handle, const char *sym);
static void mojoelf_unloader(void *handle);

const MOJOELF_Callbacks mojoelf_callbacks = {
    mojoelf_loader,
    mojoelf_resolver,
    mojoelf_unloader
};


static int find_soname_file_by_abspath(const char *path, const char *soname)
{
    if (path == NULL)
        return -1;
    const size_t len = strlen(path) + strlen(soname) + 1;
    char *fullpath = (char *) alloca(len);
    snprintf(fullpath, len, "%s/%s", path, soname);
    return open(soname, O_RDONLY);
} // find_soname_file_by_abspath

static int find_soname_file_in_path_list(const char *soname, const char *_list)
{
    if (_list == NULL)
        return -1;

    const size_t len = strlen(_list) + 1;
    char *list = (char *) alloca(len);
    char *ptr = list;
    int fd = -1;

    strcpy(list, _list);

    while (*ptr)
    {
        char *sep = strchr(ptr+1, '/');
        if (sep == NULL)
            break;
        *sep = '\0';
        fd = find_soname_file_by_abspath(ptr, soname);
        if (fd != -1)
            return fd;
        ptr = sep + 1;
    } // while

    return find_soname_file_by_abspath(ptr, soname);
} // find_soname_file_in_path_list

static int find_soname_file(const char *soname, const char *rpath, const char *runpath)
{
    int fd = -1;

    if (strchr(soname, '/'))  // is this a path already? We're done.
        return open(soname, O_RDONLY);

    // Linux dlopen() ignores rpath if runpath is set (because rpath
    //  otherwise trumps LD_LIBRARY_PATH).
    if (runpath)
        rpath = NULL;

    if ((fd = find_soname_file_in_path_list(soname, rpath)) != -1)
        return fd;
    if ((fd = find_soname_file_in_path_list(soname, ld_library_path)) != -1)
        return fd;
    if ((fd = find_soname_file_in_path_list(soname, runpath)) != -1)
        return fd;

    STUBBED("this would check /etc/ld.so.cache on Linux here.");

    if ((fd = find_soname_file_by_abspath("/lib", soname)) != -1)
        return fd;
    if ((fd = find_soname_file_by_abspath("/usr/lib", soname)) != -1)
        return fd;

    return -1;  // not found.
} // find_soname_file

static LoadedLibrary *allocate_loaded_lib(const char *soname, void *handle)
{
    LoadedLibrary *lib = (LoadedLibrary *) malloc(sizeof (LoadedLibrary));
    if (lib == NULL)
        return NULL;

    char *socpy = strdup(soname);
    if (socpy == NULL)
    {
        free(lib);
        return NULL;
    } // if

    lib->soname = socpy;
    lib->handle = handle;
    lib->refcount = 1;
    if (hash_insert(loaded_ELFs, socpy, lib) == 1)
    {
        //printf("Loaded ELF soname '%s'!\n", soname);
        return lib;
    } // if

    free(socpy);
    free(lib);
    return NULL;
} // allocate_loaded_lib

static void *mojoelf_loader(const char *soname, const char *rpath, const char *runpath)
{
    LoadedLibrary *lib = NULL;

    // see if there's an ELF file already loaded we can use.
    STUBBED("thread safety!"); // protect the hash and the LoadedLibrary refcount.
    const void *value = NULL;
    if (hash_find(loaded_ELFs, soname, &value))
    {
        //printf("Ref'ing ELF soname '%s'!\n", soname);
        lib = (LoadedLibrary *) value;
        lib->refcount++;
        return lib;
    } // if

    // we always provide glibc entry points.
    if (strcmp(soname, "libc.so.6") == 0)
        return allocate_loaded_lib(soname, NULL);

    //printf("Trying to load ELF soname '%s'!\n", soname);
    int fd = find_soname_file(soname, rpath, runpath);
    if (fd != -1)
    {
        struct stat statbuf;
        if (fstat(fd, &statbuf) == 0)
        {
            const size_t len = (size_t) statbuf.st_size;
	        void *ptr = mmap(NULL, len, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
            close(fd);
            fd = -1;
            if (ptr != ((void *) MAP_FAILED))
            {
                void *handle = MOJOELF_dlopen_mem(ptr, len, &mojoelf_callbacks);
                munmap(ptr, len);
                if (handle != NULL)
                {
                    lib = allocate_loaded_lib(soname, handle);
                    if (lib != NULL)
                        return lib;
                    MOJOELF_dlclose(handle);
                } // if
            } // if
        } // if

        if (fd != -1)
            close(fd);
    } // if

    // oh well, we don't have it.
    if (report_missing_symbols)
        printf("Missing dependency: %s\n", soname);

    if ((report_missing_symbols) || (run_with_missing_symbols))
    {
        dependencies_missing++;
        return allocate_loaded_lib(soname, NULL);  // just say we offer it, even though we don't.
    } // if

    return NULL;
} // mojoelf_loader


void missing_symbol_called(const char *missing_symbol);

static void *mojoelf_resolver(void *handle, const char *sym)
{
    LoadedLibrary *lib = (LoadedLibrary *) handle;
    const void *addr = NULL;

    if (lib != NULL)
    {
        if (lib->handle == NULL)  // something built-in, like libc.
            hash_find(resolver_symbols, sym, &addr);
        else // check something we dlopened as a dependency.
            addr = MOJOELF_dlsym(lib->handle, sym);
    } // if

    else  // uh oh, we couldn't satify this dependency!
    {
        // don't ever supply this; it's a weak symbol used by gprof and should resolve to NULL.
        if (strcmp(sym, "__gmon_start__") == 0)
            return NULL;

        // don't ever supply this; it's a weak symbol used by gcj (GNU's Java native compiler) and should resolve to NULL.
        if (strcmp(sym, "_Jv_RegisterClasses") == 0)
            return NULL;

        if (report_missing_symbols)
            printf("Missing symbol: %s\n", sym);

        if ((report_missing_symbols) || (run_with_missing_symbols))
        {
            symbols_missing++;
            #ifdef __i386__
            static void *page = NULL;
            static int pageused = 0;
            static int pagesize = 0;
            if (pagesize == 0)
                pagesize = getpagesize();

            if ((!page) || ((pagesize - pageused) < 16))
            {
                page = valloc(pagesize);
                mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);
                pageused = 0;
            } // if

            char *symcopy = strdup(sym);
            void *trampoline = page + pageused;
            char *ptr = (char *) trampoline;
            *(ptr++) = 0x55;  // push %ebp
            *(ptr++) = 0x89;  // mov %esp,%ebp
            *(ptr++) = 0xE5;  // mov %esp,%ebp
            *(ptr++) = 0x68;  // pushl immediate
            memcpy(ptr, &symcopy, sizeof (char *));
            ptr += sizeof (char *);
            *(ptr++) = 0xB8;  // movl immediate to %%eax
            const void *fn = missing_symbol_called;
            memcpy(ptr, &fn, sizeof (void *));
            ptr += sizeof (void *);
            *(ptr++) = 0xFF;  // call absolute in %%eax.
            *(ptr++) = 0xD0;
            const int trampoline_len = (int) (ptr - ((char *) trampoline));
            assert(trampoline_len <= 16);
            pageused += trampoline_len;
            #else
            #error write me.
            #endif

            return ((void *) trampoline);
        } // if
    } // else if

    return (void *) addr;
} // mojoelf_resolver

static void nuke_loadedelfs_hash(const void *key, const void *value, void *data)
{
    STUBBED("thread safety");
    LoadedLibrary *lib = (LoadedLibrary *) value;
    //printf("Unloading ELF soname '%s'!\n", lib->soname);
    assert(key == lib->soname);
    if (lib->handle != NULL)
        MOJOELF_dlclose(lib->handle);
    free(lib);
    free((void *) key);
} // nuke_loadedelfs_hash

static void mojoelf_unloader(void *handle)
{
    STUBBED("thread safety");
    LoadedLibrary *lib = (LoadedLibrary *) handle;
    //printf("Unref'ing ELF soname '%s'!\n", lib->soname);
    assert(lib->refcount > 0);
    lib->refcount--;
    if (lib->refcount == 0)
        hash_remove(loaded_ELFs, lib->soname);
} // mojoelf_unloader

static void nuke_resolver_hash(const void *key, const void *value, void *data)
{
    // !!! FIXME: write me
} // nuke_resolver_hash

int insert_symbol(const char *sym, void *addr)
{
    return (hash_insert(resolver_symbols, sym, addr) == 1);
} // insert_symbol

int build_trampolines(void);  // !!! FIXME: boooo
int mojoelf_resolver_init(void)
{
    resolver_symbols = hash_create(NULL, hash_hash_string,
                                   hash_keymatch_string, nuke_resolver_hash);
    if (resolver_symbols == NULL)
        goto fail;

    loaded_ELFs = hash_create(NULL, hash_hash_string, hash_keymatch_string,
                              nuke_loadedelfs_hash);
    if (loaded_ELFs == NULL)
        goto fail;

    if (!build_trampolines())
        goto fail;

    return 1;  // okay!

fail:
    if (resolver_symbols != NULL)
        hash_destroy(resolver_symbols);
    resolver_symbols = NULL;

    if (loaded_ELFs != NULL)
        hash_destroy(loaded_ELFs);
    loaded_ELFs = NULL;

    return 0;
} // mojoelf_resolver_init


void mojoelf_resolver_deinit(void)
{
    hash_destroy(resolver_symbols);
    resolver_symbols = NULL;
    hash_destroy(loaded_ELFs);
    loaded_ELFs = NULL;
} // mojoelf_resolver_deinit


// looks a lot like main(), huh?  :)
typedef void(*EntryFn)(int,char**,char**);

int main(int argc, char **argv, char **envp)
{
    char *elf = NULL;
    int startarg = 1;
    int i;

    ld_library_path = getenv("LD_LIBRARY_PATH");

    for (i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0)
        {
            i = argc;   // this forces the usage text. Lame way to do this.
            break;
        } // if

        else if (strcmp(arg, "--report-missing-symbols") == 0)
        {
            report_missing_symbols = 1;
            continue;
        } // else if

        else if (strcmp(arg, "--run-with-missing-symbols") == 0)
        {
            run_with_missing_symbols = 1;
            continue;
        } // else if

        else if (strcmp(arg, "--ld-library-path") == 0)
        {
            ld_library_path = argv[++i];
        } // else if

        else if (strncmp(arg, "--", 2) == 0)
        {
            fprintf(stderr, "WARNING: Unknown command line option '%s'\n", arg);
            continue;
        } // else if

        break;
    } // for

    startarg = i;

    if (argc-startarg == 0)
    {
        fprintf(stderr, "USAGE: %s [...opts...] <ELFfile> [...cmdline...]\n", argv[0]);
        fprintf(stderr, "    Valid options:\n");
        fprintf(stderr, "       --help\n");
        fprintf(stderr, "       --report-missing-symbols\n");
        fprintf(stderr, "       --run-with-missing-symbols\n");
        fprintf(stderr, "       --ld-library-path <path>\n");
        fprintf(stderr, "\n");
        return 1;
    } // if

    if (!mojoelf_resolver_init())
    {
        fprintf(stderr, "Failed to initialize. Out of memory?\n");
        return 1;
    } // if

    elf = argv[startarg];
    program_invocation_name = elf;

// !!! FIXME: use our mmap() code instead.
    void *lib = MOJOELF_dlopen_file(argv[startarg], &mojoelf_callbacks);
    if (lib == NULL)
    {
        fprintf(stderr, "Failed to load %s: %s\n", elf, MOJOELF_dlerror());
        mojoelf_resolver_deinit();
        return 1;
    } // if

    if ((symbols_missing) || (dependencies_missing))
    {
        if (report_missing_symbols)
        {
            printf("%d dependencies missing, %d symbols missing.\n",
                   dependencies_missing, symbols_missing);
        } // if

        if (!run_with_missing_symbols)
            return 1;

        fprintf(stderr, "\n\nWARNING: You are missing symbols but running anyhow!\n");
        fprintf(stderr, "WARNING: This might lead to crashes!\n\n");
    } // if

    EntryFn entry = (EntryFn) MOJOELF_getentry(lib);
    if (entry == NULL)
    {
        fprintf(stderr, "No entry point in %s\n", elf);
        MOJOELF_dlclose(lib);
        mojoelf_resolver_deinit();
        return 1;
    } // if

    //printf("About to call entry point at %p\n", entry);

    #if defined(__i386__)
    __asm__ __volatile__ (
            "pushl %%eax \n\t"  // !!! FIXME: is this stack-allocated like argv?

            // Need to copy argv array to the stack
            //  Linux entry point expects the array, not a pointer to it,
            //  to be at the start of the stack.
            "movl %%ecx,%%ebp  \n\t"  // save a copy of ecx; loop clobbers it.
            "subl %%ecx,%%esp  \n\t"  // make room for argv array.
            "0:  \n\t"
            "movl (%%edx),%%ebx  \n\t"
            "movl %%ebx,(%%esp)  \n\t"
            "addl $4,%%edx  \n\t"
            "addl $4,%%esp  \n\t"
            "subl $4,%%ecx  \n\t"
            "cmpl $0,%%ecx  \n\t"
            "jnz 0b  \n\t"
            "subl %%ebp,%%esp  \n\t"  // move back again.

            "pushl %%esi \n\t"   // this will be argc in the entry point.

            // Clear all the registers.
            // Store the entry point in %%ebp, since the entry point
            //  will clears that register.
            "movl %%edi,%%ebp  \n\t"
            "xorl %%eax,%%eax  \n\t"
            "xorl %%ebx,%%ebx  \n\t"
            "xorl %%ecx,%%ecx  \n\t"
            "xorl %%edx,%%edx  \n\t"
            "xorl %%esi,%%esi  \n\t"
            "xorl %%edi,%%edi  \n\t"

            // Tail call into the entry point! We're in Linux land!
            "jmpl *%%ebp \n\t"
        : // no outputs
        : "D" (entry), "a" (envp), "d" (argv+startarg), "S" (argc-startarg),
          "c" (((argc-startarg)+1)*sizeof (char*))
        : "memory", "cc"
    );
    #elif 0 // !!! FIXME defined(__x86_64__)
    __asm__ __volatile__ (
            "pushq %%rax \n\t"
            "pushq %%rdx \n\t"
            "pushq %%rcx \n\t"
            "xorq %%rax,%%rax  \n\t"
            "xorq %%rbx,%%rbx  \n\t"
            "xorq %%rcx,%%rcx  \n\t"
            "xorq %%rdx,%%rdx  \n\t"
            "xorq %%rsi,%%rsi  \n\t"
            "jmpq *%%rdi \n\t"
        : // no outputs
        : "D" (entry), "a" (envp), "d" (argv+1), "c" (argc)
        : "memory", "cc"
    );
    #else
    #error Please define your platform.
    #endif

    fprintf(stderr, "ELF binary shouldn't have returned to our main()!\n");
    MOJOELF_dlclose(lib);
    mojoelf_resolver_deinit();
    return 1;  // probably won't hit this.
} // main

// end of macelf.c ...

