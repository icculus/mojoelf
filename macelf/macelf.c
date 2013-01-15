#include <stdio.h>

#include "mojoelf.h"
#include "hashtable.h"

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

    entry(argc-1, argv+1, envp);

    fprintf(stderr, "ELF binary shouldn't have returned to our main()!\n");
    MOJOELF_dlclose(lib);
    macosx_resolver_deinit();
    return 1;  // probably won't hit this.
} // main

// end of macelf.c ...

