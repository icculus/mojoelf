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

