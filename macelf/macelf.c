#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "mojoelf.h"
#include "hashtable.h"

static int run_with_missing_symbols = 0;
static int report_missing_symbols = 0;
static int symbols_missing = 0;

char *program_invocation_name = NULL;

static HashTable *resolver_symbols = NULL;

void missing_symbol_called(const char *missing_symbol);

void *macosx_resolver(const char *sym)
{
    const void *ptr = NULL;
    if (!hash_find(resolver_symbols, sym, &ptr))
    {
        if (report_missing_symbols)
            printf("Missing symbol: %s\n", sym);

        if ((report_missing_symbols) || (run_with_missing_symbols))
        {
            symbols_missing++;
            if (strcmp(sym, "__gmon_start__") == 0)
                return NULL;  // !!! FIXME: this is just to prevent crash, as MOJOELF_dlopen() calls this before returning.

            #ifdef __i386__
            char *symcopy = strdup(sym);
            void *trampoline = valloc(16);  // !!! FIXME: pack these into one page.
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
            mprotect(trampoline, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
            #else
            #error write me.
            #endif

            return ((void *) trampoline);
        } // if
    } // if

    return (void *) ptr;
} // macosx_resolver


static void nuke_resolver_hash(const void *key, const void *value, void *data)
{
    // !!! FIXME: write me
} // nuke_resolver_hash

int insert_symbol(const char *sym, void *addr)
{
    return (hash_insert(resolver_symbols, sym, addr) == 1);
} // insert_symbol

int build_trampolines(void);  // !!! FIXME: boooo
int macosx_resolver_init(void)
{
    resolver_symbols = hash_create(NULL, hash_hash_string,
                                   hash_keymatch_string, nuke_resolver_hash);
    if (resolver_symbols == NULL)
        return 0;

    if (build_trampolines())
        return 1;  // okay!

    // failed.
    hash_destroy(resolver_symbols);
    resolver_symbols = NULL;
    return 0;
} // macosx_resolver_init


void macosx_resolver_deinit(void)
{
    hash_destroy(resolver_symbols);
    resolver_symbols = NULL;
} // macosx_resolver_deinit


// looks a lot like main(), huh?  :)
typedef void(*EntryFn)(int,char**,char**);

int main(int argc, char **argv, char **envp)
{
    char *elf = NULL;
    int startarg = 1;
    int i;
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
        } // if

        else if (strcmp(arg, "--run-with-missing-symbols") == 0)
        {
            run_with_missing_symbols = 1;
            continue;
        } // if

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
        fprintf(stderr, "\n");
        return 1;
    } // if

    if (!macosx_resolver_init())
    {
        fprintf(stderr, "Failed to initialize. Out of memory?\n");
        return 1;
    } // if

    elf = argv[startarg];
    program_invocation_name = elf;
    void *lib = MOJOELF_dlopen_file(argv[startarg], macosx_resolver);
    if (lib == NULL)
    {
        fprintf(stderr, "Failed to load %s: %s\n", elf, MOJOELF_dlerror());
        macosx_resolver_deinit();
        return 1;
    } // if

    if (report_missing_symbols)
    {
        printf("%d symbols missing\n", symbols_missing);
        if (!run_with_missing_symbols)
            return 1;
    } // if

    if ((run_with_missing_symbols) && (symbols_missing))
    {
        fprintf(stderr, "\n\nWARNING: You are missing symbols but running anyhow!\n");
        fprintf(stderr, "WARNING: This might lead to crashes!\n\n");
    } // if

    EntryFn entry = (EntryFn) MOJOELF_getentry(lib);
    if (entry == NULL)
    {
        fprintf(stderr, "No entry point in %s\n", elf);
        MOJOELF_dlclose(lib);
        macosx_resolver_deinit();
        return 1;
    } // if

    printf("About to call entry point at %p\n", entry);

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
    macosx_resolver_deinit();
    return 1;  // probably won't hit this.
} // main

// end of macelf.c ...

