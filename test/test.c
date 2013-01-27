/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdio.h>
#include "mojoelf.h"

// This opens a shared library, puts it in memory, "loads" it, calls into it,
//  which calls back into us.

// For expedience, we just #include the .c file.
#define MOJOELF_SUPPORT_DLERROR 1
#define MOJOELF_SUPPORT_DLOPEN_FILE 1
#define MOJOELF_ALLOW_SYSTEM_RESOLVE 1
#define MOJOELF_REDUCE_LIBC_DEPENDENCIES 1
#include "mojoelf.c"

static const char *test_person(void)
{
    static const char *names[] = { "Alice", "Bob", "Carl", "David", "Eve", 0 };
    static int calls = 0;
    const char *retval = names[calls++];
    if (names[calls] == NULL)
        calls = 0;
    return retval;
} // test_person

static void *test_resolver(const char *sym)
{
    if (strcmp(sym, "person") == 0)
        return test_person;
    return NULL;
} // test_resolver

int main(int argc, char **argv)
{
    int (*hello)(const int people_count) = NULL;
    void *lib = NULL;
    int rc;
    int i;

    for (i = 1; i < argc; i++)
    {
        printf("opening '%s'...\n", argv[i]);
        lib = MOJOELF_dlopen_file(argv[i], NULL, test_resolver);
        if (lib == NULL)
            printf("failed '%s'! (%s)\n", argv[i], MOJOELF_dlerror());
        else
        {
            printf("loaded '%s'!\n", argv[i]);
            hello = MOJOELF_dlsym(lib, "hello");
            if (hello == NULL)
                printf("Couldn't find a 'hello' function in '%s'.\n", argv[i]);
            else
            {
                printf("Found 'hello' function at %p. Calling...\n", hello);
                rc = hello(8);
                printf("...back from function call! (rc==%d)\n", rc);
            } // else
            printf("closing '%s'...\n", argv[i]);
            MOJOELF_dlclose(lib);
            printf("closed '%s'!\n", argv[i]);
        } // else
    } // for

    return 0;
} // main

// end of test.c ...

