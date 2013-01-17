#include <stdio.h>

#include "mojoelf.h"
#include "hashtable.h"

char *program_invocation_name = NULL;

static HashTable *resolver_symbols = NULL;

void *macosx_resolver(const char *sym)
{
    const void *ptr = NULL;
    if (!hash_find(resolver_symbols, sym, &ptr))
        return NULL;
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
    if (argc <= 1)
    {
        fprintf(stderr, "USAGE: %s <ELFfile> [...command line args...]\n", argv[0]);
        return 1;
    } // if

    if (!macosx_resolver_init())
    {
        fprintf(stderr, "Failed to initialize. Out of memory?\n");
        return 1;
    } // if

    program_invocation_name = argv[1];
    void *lib = MOJOELF_dlopen_file(argv[1], macosx_resolver);
    if (lib == NULL)
    {
        fprintf(stderr, "Failed to load %s: %s\n", argv[1], MOJOELF_dlerror());
        macosx_resolver_deinit();
        return 1;
    } // if

    EntryFn entry = (EntryFn) MOJOELF_getentry(lib);
    if (entry == NULL)
    {
        fprintf(stderr, "No entry point in %s\n", argv[1]);
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
        : "D" (entry), "a" (envp), "d" (argv+1), "S" (argc-1),
          "c" (argc*sizeof (char*))
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

