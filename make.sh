#!/bin/sh

set -e
set -x

gcc -DMOJOELF_TEST=1 -Wall -O0 -g -o mojoelf mojoelf.c
gcc -fPIC -shared -Wall -O0 -g -o hello.so hello.c

