mkdir -p bin
gcc -o bin/mojoelf.o \
    -c -arch i386 \
    -I.. -O0 -ggdb3 -Wall -pipe \
    -DMOJOELF_SUPPORT_DLERROR=1 \
    -DMOJOELF_SUPPORT_DLOPEN_FILE=1 \
    -DMOJOELF_ALLOW_SYSTEM_RESOLVE=0 \
    -DMOJOELF_REDUCE_LIBC_DEPENDENCIES=0 \
    ../mojoelf.c

gcc -o bin/mactrampolines.o \
    -c -arch i386 -mstackrealign \
    -I.. -I. -O0 -ggdb3 -Wall -pipe \
    ./mactrampolines.c

gcc -o bin/macelf.o \
    -c -arch i386 \
    -I.. -I. -O0 -ggdb3 -Wall -pipe \
    ./macelf.c

gcc -o bin/hashtable.o \
    -c -arch i386 \
    -I.. -I. -O0 -ggdb3 -Wall -pipe \
    ./hashtable.c

gcc -arch i386 -o macelf bin/mojoelf.o bin/mactrampolines.o bin/macelf.o bin/hashtable.o -liconv

