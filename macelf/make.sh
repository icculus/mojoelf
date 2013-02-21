# MojoELF; load ELF binaries from a memory buffer.
#
# Please see the file LICENSE.txt in the source's root directory.
#
#  This file written by Ryan C. Gordon.

#ELFARCH=x86_64
ELFARCH=i386

rm -rf bin
mkdir -p bin
clang -o bin/mojoelf.o \
    -arch $ELFARCH -c \
    -I.. -O0 -g -Wall -pipe \
    -DMOJOELF_SUPPORT_DLERROR=1 \
    -DMOJOELF_SUPPORT_DLOPEN_FILE=1 \
    -DMOJOELF_ALLOW_SYSTEM_RESOLVE=0 \
    -DMOJOELF_REDUCE_LIBC_DEPENDENCIES=0 \
    ../mojoelf.c

clang -o bin/mactrampolines.o \
    -arch $ELFARCH -c -mstackrealign \
    -I.. -I. -O0 -g -Wall -pipe \
    ./mactrampolines.c

clang -o bin/mactrampolines_sdl12.o \
    -arch $ELFARCH -c -mstackrealign \
    -I/usr/local/include/SDL \
    -I.. -I. -O0 -g -Wall -pipe \
    ./mactrampolines_sdl12.m

cd tools
perl -w ./dumpglfn.pl >../mactrampolines_opengl.h
cd ..
clang -o bin/mactrampolines_opengl.o \
    -arch $ELFARCH -c -mstackrealign \
    -I.. -I. -I/usr/X11/include -O0 -g -Wall -pipe \
    ./mactrampolines_opengl.c

clang -o bin/mactrampolines_openal.o \
    -arch $ELFARCH -c -mstackrealign \
    -I.. -I. -O0 -g -Wall -pipe \
    ./mactrampolines_openal.c

clang -o bin/mactrampolines_x11.o \
    -arch $ELFARCH -c -mstackrealign \
    -I.. -I. -I/usr/X11/include -O0 -g -Wall -pipe \
    ./mactrampolines_x11.c

clang -o bin/macelf.o \
    -arch $ELFARCH -c \
    -I.. -I. -O0 -g -Wall -pipe \
    ./macelf.c

clang -o bin/hashtable.o \
    -arch $ELFARCH -c \
    -I.. -I. -O0 -g -Wall -pipe \
    ./hashtable.c

#-image_base 0x3000000
clang -pagezero_size 0x1000 -arch $ELFARCH -g -o macelf bin/*.o -liconv -framework Cocoa -framework Carbon

# end of make.sh ...

