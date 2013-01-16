mkdir -p bin
clang -o bin/mojoelf.o \
    -arch i386 -c \
    -I.. -O0 -g -Wall -pipe \
    -DMOJOELF_SUPPORT_DLERROR=1 \
    -DMOJOELF_SUPPORT_DLOPEN_FILE=1 \
    -DMOJOELF_ALLOW_SYSTEM_RESOLVE=0 \
    -DMOJOELF_REDUCE_LIBC_DEPENDENCIES=0 \
    ../mojoelf.c

clang -o bin/mactrampolines.o \
    -arch i386 -c -mstackrealign \
    -I.. -I. -O0 -g -Wall -pipe \
    ./mactrampolines.c

clang -o bin/macelf.o \
    -arch i386 -c \
    -I.. -I. -O0 -g -Wall -pipe \
    ./macelf.c

clang -o bin/hashtable.o \
    -arch i386 -c \
    -I.. -I. -O0 -g -Wall -pipe \
    ./hashtable.c

clang -arch i386 -g -o macelf bin/mojoelf.o bin/mactrampolines.o bin/macelf.o bin/hashtable.o -liconv

