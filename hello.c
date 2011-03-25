/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdio.h>

const char *person(void);

int hello(const int people_count)
{
    int i;
    for (i = 0; i < people_count; i++)
        printf("Hello, %s!\n", person());
    return people_count + 10;
} // hello

// end of hello.c ...

