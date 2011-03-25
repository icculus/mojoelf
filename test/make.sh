#!/bin/sh

set -e
set -x

#gcc -Wall -O0 -ggdb3 -o mojoelf mojoelf.c -ldl
gcc -Wall -O0 -ggdb3 -I.. -o test test.c -ldl
gcc -Wall -O0 -ggdb3 -I.. -I/usr/include/SDL -o testsdl testsdl.c -ldl
gcc -fPIC -shared -Wall -O0 -g -o hello.so hello.c

