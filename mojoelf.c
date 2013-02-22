/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include "mojoelf.h"

// ELF specifications: http://refspecs.freestandards.org/elf/

// If not defined, force to current x86/x86_64 Linux OSABI and version.
#ifndef MOJOELF_OSABI
#define MOJOELF_OSABI 0  // x86 Linux uses ELFOSABI_SYSV, not ELFOSABI_LINUX!
#endif

#ifndef MOJOELF_ALT_OSABI
#define MOJOELF_ALT_OSABI 3  // ...but some binaries _do_ use ELFOSABI_LINUX.
#endif

#ifndef MOJOELF_OSABIVERSION
#define MOJOELF_OSABIVERSION 0
#endif

#ifndef MOJOELF_SUPPORT_DLERROR
#define MOJOELF_SUPPORT_DLERROR 1
#endif

#ifndef MOJOELF_SUPPORT_DLOPEN_FILE
#define MOJOELF_SUPPORT_DLOPEN_FILE 1
#endif

#ifndef MOJOELF_REDUCE_LIBC_DEPENDENCIES
#define MOJOELF_REDUCE_LIBC_DEPENDENCIES 0
#endif

#if (MOJOELF_SUPPORT_DLERROR && MOJOELF_SUPPORT_DLOPEN_FILE)
#include <errno.h>
#else
#ifdef errno
#undef errno
#endif
#define errno YOU_NEEDED_TO_INCLUDE_ERRNO_H___BUT_DIDNT
#endif

#if 1
#define dbgprintf(x) do {} while (0)
#else
#define dbgprintf(x) printf x
#endif

typedef intptr_t intptr;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uintptr_t uintptr;

#if defined(__i386__)
    #define MOJOELF_MACHINE_TYPE 3   // EM_386
    #define MOJOELF_32BIT 1
    #define MOJOELF_64BIT 0
    #define MOJOELF_LSB 1
    #define MOJOELF_MSB 0
#elif defined(__x86_64__)
    #define MOJOELF_MACHINE_TYPE 62  // EM_AMD64 (aka: EM_X86_64)
    #define MOJOELF_32BIT 0
    #define MOJOELF_64BIT 1
    #define MOJOELF_LSB 1
    #define MOJOELF_MSB 0
#else
    #error Please define your platform.
#endif

#if ((MOJOELF_LSB + MOJOELF_MSB) != 1)
    #error Your platform byteorder is defined incorrectly.
#endif

#if MOJOELF_LSB
    #define MOJOELF_ELFDATA 1  // ELFDATA2LSB
#elif MOJOELF_MSB
    #define MOJOELF_ELFDATA 2  // ELFDATA2MSB
#else
    #error Please define your platform.
#endif

#if ((MOJOELF_32BIT + MOJOELF_64BIT) != 1)
    #error Your platform wordsize is defined incorrectly.
#endif

#if MOJOELF_32BIT
    #define MOJOELF_PAGESIZE 4096
    #define MOJOELF_ELFCLASS 1  // ELFCLASS32
    #define MOJOELF_SIZEOF_ELF_HEADER 52
    #define MOJOELF_SIZEOF_PROGRAM_HEADER 32
    #define MOJOELF_SIZEOF_SECTION_HEADER 40
    #define MOJOELF_SIZEOF_RELENT 8
    #define MOJOELF_SIZEOF_RELAENT 12
    #define MOJOELF_SIZEOF_SYMENT 16
    #define ELF_R_SYM(x) (uint32) ((x) >> 8)
    #define ELF_R_TYPE(x) (uint32) ((x) & 0xFF)
#elif MOJOELF_64BIT
    #define MOJOELF_PAGESIZE 4096
    #define MOJOELF_ELFCLASS 2  // ELFCLASS64
    #define MOJOELF_SIZEOF_ELF_HEADER 64
    #define MOJOELF_SIZEOF_PROGRAM_HEADER 56
    #define MOJOELF_SIZEOF_SECTION_HEADER 64
    #define MOJOELF_SIZEOF_RELENT 16
    #define MOJOELF_SIZEOF_RELAENT 24
    #define MOJOELF_SIZEOF_SYMENT 24
    #define ELF_R_SYM(i) ((uint32) ((i) >> 32))
    #define ELF_R_TYPE(i) ((uint32) ((i) & 0xFFFFFFFF))
#else
    #error Please define your platform.
#endif


#if !MOJOELF_SUPPORT_DLERROR
    #define set_dlerror(x) do {} while (0)
#else
    static const char *dlerror_msg = NULL;
    static inline void set_dlerror(const char *msg) { dlerror_msg = msg; }

    const char *MOJOELF_dlerror(void)
    {
        const char *retval = dlerror_msg;
        dlerror_msg = NULL;
        return retval;
    } // MOJOELF_dlerror
#endif

// The usual ELF defines from the spec...
#define ELF_ST_BIND(i) ((i) >> 4)
#define ET_EXEC 2
#define ET_DYN 3
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_STRTAB 5
#define DT_STRSZ 10
#define DT_SYMTAB 6
#define DT_SYMENT 11
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_RPATH 15
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_JMPREL 23
#define DT_PLTREL 20
#define DT_PLTRELSZ 2
#define DT_INIT 12
#define DT_FINI 13
#define DT_INIT_ARRAY 25
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAY 25
#define DT_FINI_ARRAYSZ 28
#define DT_RUNPATH 29
#define SHT_NOBITS 8
#define SHT_DYNSYM 11
#define SHN_UNDEF 0
#define SHN_ABS 0xFFF1
#define STB_WEAK 2
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_OSABIVER 8

// Warning: These may change for other architectures!
#define R_NONE 0
#define R_32 1
#define R_PC32 2
#define R_COPY 5
#define R_GLOB_DATA 6
#define R_JUMP_SLOT 7
#define R_RELATIVE 8

typedef struct ElfHeader
{
    uint8 e_ident[16];
    uint16 e_type;
    uint16 e_machine;
    uint32 e_version;
    uintptr e_entry;
    uintptr e_phoff;
    uintptr e_shoff;
    uint32 e_flags;
    uint16 e_ehsize;
    uint16 e_phentsize;
    uint16 e_phnum;
    uint16 e_shentsize;
    uint16 e_shnum;
    uint16 e_shstrndx;
} ElfHeader;

typedef struct ElfProgram
{
    uint32 p_type;
    #if MOJOELF_64BIT  // 64-bit ABI moved it here for alignment purposes. :/
    uint32 p_flags;
    #endif
    uintptr p_offset;
    uintptr p_vaddr;
    uintptr p_paddr;
    uintptr p_filesz;
    uintptr p_memsz;
    #if MOJOELF_32BIT
    uint32 p_flags;
    #endif
    uintptr p_align;
} ElfProgram;

typedef struct ElfSection
{
    uint32 sh_name;
    uint32 sh_type;
    uintptr sh_flags;
    uintptr sh_addr;
    uintptr sh_offset;
    uintptr sh_size;
    uint32 sh_link;
    uint32 sh_info;
    uintptr sh_addralign;
    uintptr sh_entsize;
} ElfSection;

typedef struct ElfSymTable
{
    uint32 st_name;
    #if MOJOELF_64BIT  // 64-bit ABI moved it here for alignment purposes. :/
    uint8 st_info;
    uint8 st_other;
    uint16 st_shndx;
    #endif
    uintptr st_value;
    uintptr st_size;
    #if MOJOELF_32BIT
    uint8 st_info;
    uint8 st_other;
    uint16 st_shndx;
    #endif
} ElfSymTable;

typedef struct ElfRel
{
    uintptr r_offset;
    uintptr r_info;
} ElfRel;

typedef struct ElfRelA
{
    uintptr r_offset;
    uintptr r_info;
    intptr r_addend;
} ElfRelA;

typedef struct ElfSymbols
{
    char *sym;
    void *addr;
} ElfSymbols;

typedef struct ElfHandle  // this is what MOJOELF_dlopen_*() returns.
{
    int mmaps_count;
    void *mmapaddr;
    size_t mmaplen;
    int syms_count;
    ElfSymbols *syms;
    void *entry;
    void *fini;          // single destructor
    void **fini_array;   // fini array function in shared library.
    int fini_array_count;  // number of functions in the fini array.
    int dlopens_count;
    void **dlopens;
    MOJOELF_UnloaderCallback unloader;  // unloader callback.
} ElfHandle;


// Actual shared object loading happens here.

#if !MOJOELF_REDUCE_LIBC_DEPENDENCIES
#define Strcmp(a,b) strcmp(a,b)
#define Strcpy(a,b) strcpy(a,b)
#define Memzero(buf,len) memset(buf, '\0', len)
#define Memcopy(dst,src,len) memcpy(dst,src,len)
#else
static inline int Strcmp(const char *_a, const char *_b)
{
    while (*_a)
    {
        const char a = *(_a++);
        const char b = *(_b++);
        if (a < b)
            return -1;
        else if (a > b)
            return -1;
        else if (a == 0)
            return 0;
    } // while
    return 0;
} // Strcmp

static inline void Strcpy(char *_a, const char *_b)
{
    while (1)
    {
        const char b = *(_b++);
        *(_a++) = b;
        if (b == '\0')
            return;
    } // while    
} // Strcpy

static inline void Memzero(void *buf, size_t len)
{
    uintptr *ptrnative = (uintptr *) buf;
    uint8 *ptrbyte;

    while (len >= sizeof (uintptr))
    {
        *(ptrnative++) = 0;
        len -= sizeof (uintptr);
    } // while

    ptrbyte = (uint8 *) ptrnative;
    while (len--)
        *(ptrbyte++) = '\0';
} // Memzero

static inline void Memcopy(void *dst, const void *src, size_t len)
{
    uintptr *dptrnative = (uintptr *) dst;
    const uintptr *sptrnative = (uintptr *) src;
    uint8 *dptrbyte;
    const uint8 *sptrbyte;

    while (len >= sizeof (uintptr))
    {
        *dptrnative = *sptrnative;
        dptrnative++;
        sptrnative++;
        len -= sizeof (uintptr);
    } // while

    dptrbyte = (uint8 *) dptrnative;
    sptrbyte = (const uint8 *) sptrnative;
    while (len--)
    {
        *dptrbyte = *sptrbyte;
        dptrbyte++;
        sptrbyte++;
    } // while
} // Memzero
#endif

static inline void *Malloc(const size_t len)
{
    void *retval = calloc(1, len);
    if (retval == NULL)
        set_dlerror("Out of memory");
    return retval;
} // Malloc

typedef struct ElfDynTable
{
    uint32 d_tag;
    union
    {
        uint32 d_val;
        uintptr d_ptr;
    } d_un;
} ElfDynTable;

typedef void (*ElfInitFn)(int argc, char **argv, char **envp);
typedef void (*ElfFiniFn)(void);

// Put a bunch of state we need during dlopen() into one struct; this lets
//  us split one big function into more manageable chunks.
typedef struct ElfContext
{
    const uint8 *buf;  // buffer passed to dlopen.
    size_t buflen;   // size in bytes of buffer passed to dlopen.
    const ElfHeader *header;  // main ELF header (also, start of buffer).
    ElfHandle *retval;  // allocated handle to be returned from dlopen.
    const ElfDynTable *dyntab;  // PT_DYNAMIC tables.
    int dyntabcount;  // PT_DYNAMIC table count.
    uintptr base;  // "base address" for relative addressing.
    void *init;   // init function in shared library.
    void **init_array;   // init array function in shared library.
    int init_array_count;  // number of functions in the init array.
    size_t mmaplen;  // number of bytes we want to mmap().
    const char *strtab;  // string table for dynamic symbols.
    size_t strtablen;  // length in bytes of dynamic symbol string table.
    const ElfSymTable *symtab;  // the symbol table.
    int symtabcount;  // entries in the symbol table.
    const ElfDynTable *dyntabs[32];  // indexable pointers to dynamic tables.
    MOJOELF_LoaderCallback loader;    // loader callback.
    MOJOELF_UnloaderCallback unloader;  // unloader callback.
    MOJOELF_ResolverCallback resolver;  // resolver callback.
} ElfContext;

#define DLOPEN_FAIL(err) do { set_dlerror(err); return 0; } while (0)

static int validate_elf_header(const ElfContext *ctx)
{
    const ElfHeader *hdr = ctx->header;
    const uint8 *buf = hdr->e_ident;
    const uint8 osabi = buf[EI_OSABI];

    if (ctx->buflen < 64)
        DLOPEN_FAIL("Not enough data");
    else if ((buf[0]!=0x7F) || (buf[1]!='E') || (buf[2]!='L') || (buf[3]!='F'))
        DLOPEN_FAIL("Not an ELF file");
    else if (buf[EI_CLASS] != MOJOELF_ELFCLASS)
        DLOPEN_FAIL("Unsupported/bogus ELF class");
    else if (buf[EI_DATA] != MOJOELF_ELFDATA)
        DLOPEN_FAIL("Unsupported/bogus ELF data ordering");
    else if (buf[EI_VERSION] != 1)
        DLOPEN_FAIL("Unsupported/bogus ELF file version");
    else if ((osabi != MOJOELF_OSABI) && (osabi != MOJOELF_ALT_OSABI))
        DLOPEN_FAIL("Unsupported/bogus ELF OSABI");
    else if (buf[EI_OSABIVER] != MOJOELF_OSABIVERSION)
        DLOPEN_FAIL("Unsupported/bogus ELF OSABI");
    else if ((hdr->e_type != ET_DYN) && (hdr->e_type != ET_EXEC)) // binary or .so
        DLOPEN_FAIL("Unsupported/bogus ELF object type");
    else if (hdr->e_machine != MOJOELF_MACHINE_TYPE)
        DLOPEN_FAIL("Unsupported/bogus ELF machine type");
    else if (hdr->e_version != 1)  // "current"
        DLOPEN_FAIL("Unsupported/bogus ELF object version");
    else if (hdr->e_ehsize != sizeof (ElfHeader))
        DLOPEN_FAIL("Unsupported/bogus ELF main header size");
    else if (hdr->e_phentsize != sizeof (ElfProgram))
        DLOPEN_FAIL("Unsupported/bogus ELF program header size");
    else if (hdr->e_shentsize != sizeof (ElfSection))
        DLOPEN_FAIL("Unsupported/bogus ELF section header size");
    else if ((hdr->e_phoff + (hdr->e_phnum * hdr->e_phentsize)) > ctx->buflen)
        DLOPEN_FAIL("Bogus ELF program header offset/count");
    else if ((hdr->e_shoff + (hdr->e_shnum * hdr->e_shentsize)) > ctx->buflen)
        DLOPEN_FAIL("Bogus ELF section header offset/count");

    // we ignore the string table.

    return 1;  // all good!
} // validate_elf_header


static int validate_elf_section(const ElfContext *ctx, const ElfSection *section)
{
    if (section->sh_offset + section->sh_size > ctx->buflen)
    {
        // Spec says NOBITS may have non-zero size, but takes no space in file.
        if (section->sh_type != SHT_NOBITS)
            DLOPEN_FAIL("Bogus ELF program offset/size");
    } // if
    // we currently ignore the string table in validate_elf_header, so we
    //  can't verify this string.
    //else if ((maxstr > 0) && (sect->sh_name > maxstr))
    //    DLOPEN_FAIL("Bogus ELF section name index");
    return 1;
} // validate_elf_section


static int validate_elf_program(const ElfContext *ctx, const ElfProgram *prog)
{
    if ((prog->p_offset + prog->p_filesz) > ctx->buflen)
        DLOPEN_FAIL("Bogus ELF program offset/size");
    else if (prog->p_filesz > prog->p_memsz)
        DLOPEN_FAIL("Bogus ELF program size");
    return 1;
} // validate_elf_program


static int process_program_headers(ElfContext *ctx)
{
    // Figure out the memory range we'll need to allocate.
    const size_t offset = (size_t) ctx->header->e_phoff;
    const ElfProgram *program = (const ElfProgram *) (ctx->buf + offset);
    const int header_count = (int) ctx->header->e_phnum;
    int i;

    ctx->base = ((uintptr) -1);  // 0xFFFFFFFF or 0xFFFFFFFFFFFFFFFF

    for (i = 0; i < header_count; i++, program++)
    {
        if (!validate_elf_program(ctx, program))
            return 0;

        else if ((program->p_type == PT_LOAD) && (program->p_memsz > 0))
        {
            const size_t endaddr = program->p_vaddr + program->p_memsz;
            if (endaddr > ctx->mmaplen)
                ctx->mmaplen = endaddr;
            if (program->p_vaddr < ctx->base)
                ctx->base = program->p_vaddr;
        } // else if

        // When we see this header, take note for later.
        else if (program->p_type == PT_DYNAMIC)
        {
            const uint8 *buf = (const uint8 *) ctx->header;
            if (ctx->dyntab != NULL)  // Can there be more than one of these?!
                DLOPEN_FAIL("Multiple PT_DYNAMIC tables");
            ctx->dyntab = (const ElfDynTable *) (buf + program->p_offset);
            ctx->dyntabcount = (program->p_filesz / sizeof (ElfDynTable));
        } // else if
    } // for

    if (ctx->mmaplen == 0)
        DLOPEN_FAIL("No loadable pages");
    else if (ctx->dyntab == NULL)
        DLOPEN_FAIL("No PT_DYNAMIC table");

    // Calculate the final base address and allocation size.
    ctx->base -= (ctx->base % MOJOELF_PAGESIZE);
    ctx->mmaplen -= (size_t) ctx->base;
    ctx->mmaplen += (MOJOELF_PAGESIZE - (ctx->mmaplen % MOJOELF_PAGESIZE));

    return 1;
} // process_program_headers


static int process_section_headers(ElfContext *ctx)
{
    const size_t offset = (size_t) ctx->header->e_shoff;
    const ElfSection *section = (const ElfSection *) (ctx->buf + offset);
    const int header_count = (int) ctx->header->e_shnum;
    const ElfDynTable *dt = ctx->dyntabs[DT_SYMTAB];
    int saw_dynsym = 0;
    int i;

    for (i = 0; i < header_count; i++, section++)
    {
        if (!validate_elf_section(ctx, section))
            return 0;

        if (section->sh_type == SHT_DYNSYM)
        {
            if (dt == NULL)
                DLOPEN_FAIL("Dynamic symbol table section, but not program");
            else if ((dt->d_un.d_ptr-ctx->base) != section->sh_offset)
                DLOPEN_FAIL("Dynamic symbol table program/section mismatch");
            else if ((section->sh_offset + section->sh_size) >= ctx->buflen)
                DLOPEN_FAIL("Bogus dynamic symbol table offset/size");
            else if (section->sh_entsize != MOJOELF_SIZEOF_SYMENT)
                DLOPEN_FAIL("Bogus dynamic symbol table section entsize");
            else if (section->sh_size % MOJOELF_SIZEOF_SYMENT)
                DLOPEN_FAIL("Bogus dynamic symbol table section size");
            ctx->symtabcount = section->sh_size / MOJOELF_SIZEOF_SYMENT;
            saw_dynsym = 1;
        } // if
    } // for

    if ((!saw_dynsym) && (dt != NULL))
        DLOPEN_FAIL("Missing dynamic symbol table section");

    return 1;
} // process_section_headers


// Get the ELF programs into memory at the right place.
static int map_pages(ElfContext *ctx)
{
    // Get us an aligned block of memory where we can change page permissions.
    // This is big enough to store all the program blocks and place them at
    //  the correct relative offsets.
    // !!! FIXME: should we be mapping each section separately?
    const size_t offset = (size_t) ctx->header->e_phoff;
    const ElfProgram *program = (const ElfProgram *) (ctx->buf + offset);
    const int header_count = (int) ctx->header->e_phnum;
    const size_t mmaplen = ctx->mmaplen;
    const int mmapprot = PROT_READ | PROT_WRITE;
    const int mmapflags = MAP_ANON | MAP_PRIVATE | (ctx->base ? MAP_FIXED : 0);
    void *mmapaddr = mmap((void *) ctx->base, mmaplen, mmapprot, mmapflags, -1, 0);
    int i;

    if (mmapaddr == ((void *) MAP_FAILED))
        DLOPEN_FAIL("mmap failed");

    Memzero(mmapaddr, mmaplen);
    ctx->retval->mmapaddr = mmapaddr;
    ctx->retval->mmaplen = mmaplen;

    // Put the program blocks at the correct relative positions.
    for (i = 0; i < header_count; i++, program++)
    {
        if ((program->p_type == PT_LOAD) && (program->p_memsz > 0))
        {
            uint8 *ptr = ((uint8 *) mmapaddr) + (program->p_vaddr - ctx->base);
            Memcopy(ptr, ctx->buf + program->p_offset, program->p_filesz);
        } // if
    } // for

    // we mprotect() these pages later in the process, since fixups might want
    //  to write to memory that will eventually be marked read-only, etc.
    //  That happens in protect_pages().

    return 1;
} // map_pages

// Mark ELF pages with proper permissions.
static int protect_pages(ElfContext *ctx)
{
    const size_t offset = (size_t) ctx->header->e_phoff;
    const ElfProgram *program = (const ElfProgram *) (ctx->buf + offset);
    const int header_count = (int) ctx->header->e_phnum;
    const int mmapprot = PROT_READ | PROT_WRITE;
    void *mmapaddr = (void *) ctx->retval->mmapaddr;
    int i;

    for (i = 0; i < header_count; i++, program++)
    {
        if ((program->p_type == PT_LOAD) && (program->p_memsz > 0))
        {
            uint8 *ptr = ((uint8 *) mmapaddr) + (program->p_vaddr - ctx->base);
            const size_t len = (const size_t) program->p_memsz;
            const int prot = ((program->p_flags & 1) ? PROT_EXEC : 0)  |
                             ((program->p_flags & 2) ? PROT_WRITE : 0) |
                             ((program->p_flags & 4) ? PROT_READ : 0)  ;
            if ((prot != mmapprot) && (mprotect(ptr, len, prot) == -1))
                DLOPEN_FAIL("mprotect failed");
        } // if
    } // for

    return 1;  // all good.
} // protect_pages

static int walk_dynamic_table(ElfContext *ctx)
{
    // preliminary walkthrough of the dynamic table.
    const ElfDynTable *dyntab = ctx->dyntab;
    const ElfDynTable **dyntabs = ctx->dyntabs;
    const int dyntabcount = ctx->dyntabcount;
    uint8 *mmapaddr = (uint8 *) ctx->retval->mmapaddr;
    uintptr strtaboffset = 0;
    int i;

    for (i = 0; i < dyntabcount; i++, dyntab++)
    {
        const int tag = (int) dyntab->d_tag;

        if (tag == DT_NULL)
            continue;
        else if (tag == DT_NEEDED)
        {
            ctx->retval->dlopens_count++;
            continue;
        } // if

        if (tag < (sizeof (ctx->dyntabs) / sizeof (ctx->dyntabs[0])))
        {
            if (dyntabs[tag] != NULL)
                DLOPEN_FAIL("Illegal duplicate dynamic tables");
            dyntabs[tag] = dyntab;
        } // if
    } // for

    // okay, all the tables are indexed now, so we don't have to worry about
    //  dupes or missing fields, or ordering issues.
    // Now that we've shuffled this stuff into an index, process it.

    if (dyntabs[DT_STRTAB] == NULL)
        DLOPEN_FAIL("No DT_STRTAB table");
    else if (dyntabs[DT_STRSZ] == NULL)
        DLOPEN_FAIL("No DT_STRSZ table");
    else if (dyntabs[DT_SYMTAB] == NULL)
        DLOPEN_FAIL("No DT_SYMTAB table");
    else if (dyntabs[DT_SYMENT] == NULL)
        DLOPEN_FAIL("No DT_SYMENT table");
    else if (dyntabs[DT_SYMENT]->d_un.d_val != MOJOELF_SIZEOF_SYMENT)
        DLOPEN_FAIL("Bogus DT_SYMENT value");

    // We don't know the size of the symbol table yet. We check that later
    //  in process_section_headers()...
    ctx->symtab = (const ElfSymTable *)
                    (ctx->buf + (dyntabs[DT_SYMTAB]->d_un.d_ptr - ctx->base));

    if (dyntabs[DT_RELA])
    {
        if (dyntabs[DT_RELASZ] == NULL)
            DLOPEN_FAIL("No DT_RELASZ table");
        else if (dyntabs[DT_RELAENT] == NULL)
            DLOPEN_FAIL("No DT_RELAENT table");
        else if (dyntabs[DT_RELAENT]->d_un.d_val != MOJOELF_SIZEOF_RELAENT)
            DLOPEN_FAIL("Unsupported/bogus DT_RELAENT value");
        else if (dyntabs[DT_RELASZ]->d_un.d_val % MOJOELF_SIZEOF_RELAENT)
            DLOPEN_FAIL("Bogus DT_RELASZ value");
        else if ((dyntabs[DT_RELA]->d_un.d_ptr - ctx->base) +
                 dyntabs[DT_RELASZ]->d_un.d_val >= ctx->buflen)
            DLOPEN_FAIL("Bogus DT_RELA value");
    } // if

    if (dyntabs[DT_REL])
    {
        if (dyntabs[DT_RELSZ] == NULL)
            DLOPEN_FAIL("No DT_RELSZ table");
        else if (dyntabs[DT_RELENT] == NULL)
            DLOPEN_FAIL("No DT_RELENT table");
        else if (dyntabs[DT_RELENT]->d_un.d_val != MOJOELF_SIZEOF_RELENT)
            DLOPEN_FAIL("Unsupported/bogus DT_RELENT value");
        else if (dyntabs[DT_RELSZ]->d_un.d_val % MOJOELF_SIZEOF_RELENT)
            DLOPEN_FAIL("Bogus DT_RELSZ value");
        else if ((dyntabs[DT_REL]->d_un.d_ptr - ctx->base) >= ctx->buflen)
            DLOPEN_FAIL("Bogus DT_REL value");
    } // if

    if (dyntabs[DT_JMPREL])
    {
        if (dyntabs[DT_PLTREL] == NULL)
            DLOPEN_FAIL("No DT_PLTREL table");
        else if (dyntabs[DT_PLTRELSZ] == NULL)
            DLOPEN_FAIL("No DT_PLTRELSZ table");
        else if ((dyntabs[DT_JMPREL]->d_un.d_ptr - ctx->base) +
                 dyntabs[DT_PLTRELSZ]->d_un.d_val >= ctx->buflen)
            DLOPEN_FAIL("Bogus DT_PLTREL value");

        if (dyntabs[DT_PLTREL]->d_un.d_val == DT_REL)
        {
            if (dyntabs[DT_RELENT] == NULL)  // DT_RELENT
                DLOPEN_FAIL("No DT_RELENT table");
            else if (dyntabs[DT_RELENT]->d_un.d_val != MOJOELF_SIZEOF_RELENT)
                DLOPEN_FAIL("Unsupported/bogus DT_RELENT value");
            else if (dyntabs[DT_PLTRELSZ]->d_un.d_val % MOJOELF_SIZEOF_RELENT)
                DLOPEN_FAIL("Bogus DT_PLTRELSZ value");
        } // if

        else if (dyntabs[DT_PLTREL]->d_un.d_val == DT_RELA)
        {
            if (dyntabs[DT_RELAENT] == NULL)  // DT_RELAENT
                DLOPEN_FAIL("No DT_RELAENT table");
            else if (dyntabs[DT_RELAENT]->d_un.d_val != MOJOELF_SIZEOF_RELAENT)
                DLOPEN_FAIL("Unsupported/bogus DT_RELAENT value");
            else if (dyntabs[DT_PLTRELSZ]->d_un.d_val % MOJOELF_SIZEOF_RELAENT)
                DLOPEN_FAIL("Bogus DT_PLTRELSZ value");
        } // else if

        else
        {
            DLOPEN_FAIL("Bogus DT_PLTREL value");
        } // else
    } // if

    strtaboffset = dyntabs[DT_STRTAB]->d_un.d_ptr - ctx->base;
    ctx->strtablen = (size_t) dyntabs[DT_STRSZ]->d_un.d_val;
    ctx->strtab = (const char *) (ctx->buf + strtaboffset);
    if (ctx->strtablen == 0)  // technically this is allowed, but oh well.
        DLOPEN_FAIL("Dynamic string table is empty");
    else if ((strtaboffset + ctx->strtablen) > ctx->buflen)
        DLOPEN_FAIL("Dynamic string table has bogus offset and/or length");
    else if (ctx->strtab[0] != '\0')
        DLOPEN_FAIL("Dynamic string table doesn't start with null byte");
    else if (ctx->strtab[ctx->strtablen - 1] != '\0')
        DLOPEN_FAIL("Dynamic string table doesn't end with null byte");

    if (dyntabs[DT_INIT])
        ctx->init = mmapaddr + (dyntabs[DT_INIT]->d_un.d_ptr-ctx->base);

    if (dyntabs[DT_FINI])
        ctx->retval->fini = mmapaddr + (dyntabs[DT_FINI]->d_un.d_ptr-ctx->base);

    if (dyntabs[DT_INIT_ARRAY])
    {
        if (dyntabs[DT_INIT_ARRAYSZ] == NULL)
            DLOPEN_FAIL("No DT_INIT_ARRAYSZ table");
        else if (dyntabs[DT_INIT_ARRAYSZ]->d_un.d_val % sizeof (void*))
            DLOPEN_FAIL("Bogus DT_INIT_ARRAYSZ value");
        else if ((dyntabs[DT_INIT_ARRAY]->d_un.d_ptr - ctx->base) +
                 dyntabs[DT_INIT_ARRAYSZ]->d_un.d_val >= ctx->buflen)
            DLOPEN_FAIL("Bogus DT_INIT_ARRAY value");
        ctx->init_array = (void **) (mmapaddr + (dyntabs[DT_INIT_ARRAY]->d_un.d_ptr-ctx->base));
        ctx->init_array_count = (int) (dyntabs[DT_INIT_ARRAYSZ]->d_un.d_val / sizeof (void*));
    } // if

    if (dyntabs[DT_FINI_ARRAY])
    {
        if (dyntabs[DT_FINI_ARRAYSZ] == NULL)
            DLOPEN_FAIL("No DT_FINI_ARRAYSZ table");
        else if (dyntabs[DT_FINI_ARRAYSZ]->d_un.d_val % sizeof (void*))
            DLOPEN_FAIL("Bogus DT_FINI_ARRAYSZ value");
        else if ((dyntabs[DT_FINI_ARRAY]->d_un.d_ptr - ctx->base) +
                 dyntabs[DT_FINI_ARRAYSZ]->d_un.d_val >= ctx->buflen)
            DLOPEN_FAIL("Bogus DT_FINI_ARRAY value");
        ctx->retval->fini_array = (void **) (mmapaddr + (dyntabs[DT_FINI_ARRAY]->d_un.d_ptr-ctx->base));
        ctx->retval->fini_array_count = (int) (dyntabs[DT_FINI_ARRAYSZ]->d_un.d_val / sizeof (void*));
    } // if

    return 1;
} // walk_dynamic_table


// We might need to load symbols in other libraries. The app-supplied
//  callbacks will be given a chance to handle symbol resolution here.
static int load_external_dependencies(ElfContext *ctx)
{
    const ElfDynTable *dyntab = ctx->dyntab;
    const int dyntabcount = ctx->dyntabcount;
    const char *rpath = NULL;
    const char *runpath = NULL;
    int dlopens_count = 0;
    int i;

    if (ctx->dyntabs[DT_RPATH] != NULL)
    {
        const size_t offset = ctx->dyntabs[DT_RPATH]->d_un.d_val;
        if (offset >= ctx->strtablen)
            DLOPEN_FAIL("Bogus DT_RPATH string");
        rpath = ctx->strtab + offset;
    } // if

    if (ctx->dyntabs[DT_RUNPATH] != NULL)
    {
        const size_t offset = ctx->dyntabs[DT_RUNPATH]->d_un.d_val;
        if (offset >= ctx->strtablen)
            DLOPEN_FAIL("Bogus DT_RUNPATH string");
        runpath = ctx->strtab + offset;
    } // if

    if (ctx->retval->dlopens_count == 0)
        return 1;  // nothing to do.

    ctx->retval->dlopens = (void **) Malloc(ctx->retval->dlopens_count * sizeof (void *));
    if (ctx->retval->dlopens == NULL)
        return 0;

    // Find the libraries to load.
    for (i = 0; i < dyntabcount; i++, dyntab++)
    {
        if (dyntab->d_tag == DT_NEEDED)
        {
            const size_t offset = dyntab->d_un.d_val;
            if (offset >= ctx->strtablen)
                DLOPEN_FAIL("Bogus DT_NEEDED string");
            else
            {
                const char *str = ctx->strtab + offset;
                void *handle = NULL;

                dbgprintf(("asking loader for \"%s\" (rpath \"%s\", runpath \"%s\") ...\n", str, rpath, runpath));
                handle = ctx->loader(str, rpath, runpath);
                if (!handle)
                    DLOPEN_FAIL("Couldn't load dependency");

                ctx->retval->dlopens[dlopens_count++] = handle;
                if (dlopens_count >= ctx->retval->dlopens_count)
                    return 1;  // done!
            } // else
        } // if
    } // for

    assert(0);
    DLOPEN_FAIL("Bug: didn't see as many DT_NEEDED on second pass");
} // load_external_dependencies


static int resolve_symbol(ElfContext *ctx, const uint32 sym, uintptr *_addr)
{
    const ElfSymTable *symbol = ctx->symtab + sym;
    const char *symstr = NULL;
    void *addr = NULL;

    if ((symbol->st_value) && ((symbol->st_value - ctx->base) > ctx->retval->mmaplen))
        DLOPEN_FAIL("Bogus symbol address");
    else if (symbol->st_name >= ctx->strtablen)
        DLOPEN_FAIL("Bogus symbol name");

    symstr = ctx->strtab + symbol->st_name;

    if (*symstr == '\0')
        addr = ((uint8 *) ctx->retval->mmapaddr) + symbol->st_value;
    else
    {
        dbgprintf(("Resolving '%s' ...\n", symstr));

        int i;
        for (i = 0; (addr == NULL) && (i < ctx->retval->dlopens_count); i++)
            addr = ctx->resolver(ctx->retval->dlopens[i], symstr);

        if (addr == NULL)
        {
            // try our own export table?
            addr = MOJOELF_dlsym(ctx->retval, symstr);
            if (addr == NULL)
            {
                addr = ctx->resolver(NULL, symstr);  // last try.
                if (addr == NULL)
                {
                    if (ELF_ST_BIND(symbol->st_info) != STB_WEAK)
                        DLOPEN_FAIL("Couldn't resolve symbol");
                } // if
            } // if
        } // if

        dbgprintf(("Resolved '%s' to %p ...\n", symstr, addr));
    } // if

    *_addr = (uintptr) addr;
    return 1;
} // resolve_symbol


static int do_fixup(ElfContext *ctx, const uint32 r_type, const uint32 r_sym,
                    const uintptr r_offset, const intptr r_addend)
{
    uint8 *mmapaddr = (uint8 *) ctx->retval->mmapaddr;
    uintptr *fixup = (uintptr *) (mmapaddr + (r_offset - ctx->base));
    uintptr addr = 0;

    if (r_sym >= ctx->symtabcount)
        DLOPEN_FAIL("Bogus symbol index");
    else if ((r_sym) && (!resolve_symbol(ctx, r_sym, &addr)))
        return 0;

    switch (r_type)
    {
        // There are way more than these, but these seem to be the
        //  only ones average libraries use.
        // Note libc.so.6 itself also seems to use: R_*_64, R_*_TPOFF64
// !!! FIXME: This is all x86-specific. Research x86_64.
        case R_COPY:
            if (!r_sym)
                DLOPEN_FAIL("Bogus copy fixup");
            // !!! FIXME: this should check the source and dest symbols,
            // !!! FIXME:  use the smaller size (but they _should_ match
            // !!! FIXME:  if everything is built correctly. Naturally, this
            // !!! FIXME:  will fail to be true eventually).
            memcpy(fixup, (void *) addr, (ctx->symtab+r_sym)->st_size);
            break;
        case R_GLOB_DATA:
        case R_JUMP_SLOT:
            *fixup = addr;
            break;
        case R_RELATIVE:
            *fixup += (uintptr) (mmapaddr + r_addend);
            break;
        case R_32:  // !!! FIXME: presumable should be (uint32), not (uintptr).
            *fixup += (uintptr) (addr + r_addend);
            break;
        case R_PC32:  // !!! FIXME: presumable should be (uint32), not (uintptr).
            *fixup += (uintptr) ((addr + r_addend) - ((uintptr) fixup));
            break;
        case R_NONE:
            break;  // do nothing.
        default:
            DLOPEN_FAIL("write me");  // haven't seen this yet.
            break;
    } // switch

    return 1;
} // do_fixup

static int fixup_rela_internal(ElfContext *ctx, const ElfDynTable *dt_rela,
                               const ElfDynTable *dt_relasz)
{
    const size_t offset = ((size_t) dt_rela->d_un.d_ptr) - ctx->base;
    const size_t relasz = (size_t) dt_relasz->d_un.d_val;
    const size_t count = relasz / MOJOELF_SIZEOF_RELAENT;
    const ElfRelA *rela = (const ElfRelA *) (ctx->buf + offset);
    int i;

    for (i = 0; i < count; i++, rela++)
    {
        if (!do_fixup(ctx, ELF_R_TYPE(rela->r_info), ELF_R_SYM(rela->r_info),
                      rela->r_offset, rela->r_addend))
            return 0;
    } // for

    return 1;
} // fixup_rela_internal

static inline int fixup_rela(ElfContext *ctx)
{
    const ElfDynTable *dt_rela = ctx->dyntabs[DT_RELA];
    const ElfDynTable *dt_relasz = ctx->dyntabs[DT_RELASZ];
    if (dt_rela == NULL)  // it's optional, we're done if it's not there.
        return 1;
    return fixup_rela_internal(ctx, dt_rela, dt_relasz);
} // fixup_rela

static int fixup_rel_internal(ElfContext *ctx, const ElfDynTable *dt_rel,
                              const ElfDynTable *dt_relsz)
{
    const size_t offset = ((size_t) dt_rel->d_un.d_ptr) - ctx->base;
    const size_t relsz = (size_t) dt_relsz->d_un.d_val;
    const size_t count = relsz / MOJOELF_SIZEOF_RELENT;
    const ElfRel *rel = (const ElfRel *) (ctx->buf + offset);
    int i;

    for (i = 0; i < count; i++, rel++)
    {
        if (!do_fixup(ctx, ELF_R_TYPE(rel->r_info), ELF_R_SYM(rel->r_info),
                      rel->r_offset, 0))
            return 0;
    } // for

    return 1;
} // fixup_rel_internal

static inline int fixup_rel(ElfContext *ctx)
{
    const ElfDynTable *dt_rel = ctx->dyntabs[DT_REL];
    const ElfDynTable *dt_relsz = ctx->dyntabs[DT_RELSZ];
    if (dt_rel == NULL)  // it's optional, we're done if it's not there.
        return 1;
    return fixup_rel_internal(ctx, dt_rel, dt_relsz);
} // fixup_rel

static inline int fixup_jmprel(ElfContext *ctx)
{
    const ElfDynTable *dt_jmprel = ctx->dyntabs[DT_JMPREL];
    const ElfDynTable *dt_jmprelsz = ctx->dyntabs[DT_PLTRELSZ];

    if (dt_jmprel == NULL)  // it's optional, we're done if it's not there.
        return 1;

    if (ctx->dyntabs[DT_PLTREL]->d_un.d_val == DT_RELA)
        return fixup_rela_internal(ctx, dt_jmprel, dt_jmprelsz);

    assert(ctx->dyntabs[DT_PLTREL]->d_un.d_val == DT_REL);
    return fixup_rel_internal(ctx, dt_jmprel, dt_jmprelsz);
} // fixup_jmprel

static int fixup_relocations(ElfContext *ctx)
{
    if (!fixup_rela(ctx))
        return 0;
    else if (!fixup_rel(ctx))
        return 0;
    else if (!fixup_jmprel(ctx))
        return 0;
    return 1;
} // fixup_relocations

static int add_exported_symbol(ElfContext *ctx, const char *sym, void *addr)
{
    // !!! FIXME: We should really use a hash table for all this.
    const int syms_count = ctx->retval->syms_count;
    ElfSymbols *syms = ctx->retval->syms;

    // We don't check for duplicates here. You get the first one in the list!

    syms[syms_count].sym = (char *) Malloc(strlen(sym) + 1);
    if (syms[syms_count].sym == NULL)
        return 0;

    Strcpy(syms[syms_count].sym, sym);
    syms[syms_count].addr = addr;
    ctx->retval->syms_count++;

    return 1;
} // add_exported_symbol


static int build_export_list(ElfContext *ctx)
{
    const ElfSymTable *symbol = ctx->symtab;
    int symcount = 0;
    int i;

    for (i = 1; i <= ctx->symtabcount; i++, symbol++)
    {
        const uintptr offset = symbol->st_value ? (symbol->st_value - ctx->base) : 0;
        void *addr = ((uint8 *) ctx->retval->mmapaddr) + offset;
        const char *symstr;

        if (offset > ctx->retval->mmaplen)
            DLOPEN_FAIL("Bogus symbol address");
        else if (symbol->st_name >= ctx->strtablen)
            DLOPEN_FAIL("Bogus symbol name");

        symstr = ctx->strtab + symbol->st_name;

        if ((*symstr != '\0') && 
            (symbol->st_shndx != SHN_UNDEF) &&
            (symbol->st_shndx != SHN_ABS) &&
            (addr != ctx->init) &&
            (addr != ctx->retval->fini))
        {
            symcount++;
        } // if
    } // for

    if (symcount == 0)
        return 1;  // nothing to do!

    ctx->retval->syms = (ElfSymbols *) Malloc(sizeof (ElfSymbols) * symcount);
    if (ctx->retval->syms == NULL)
        return 0;

    symbol = ctx->symtab;
    for (i = 1; i <= ctx->symtabcount; i++, symbol++)
    {
        const uintptr offset = symbol->st_value ? (symbol->st_value - ctx->base) : 0;
        void *addr = ((uint8 *) ctx->retval->mmapaddr) + offset;
        const char *symstr = ctx->strtab + symbol->st_name;

        if ((*symstr != '\0') && 
            (symbol->st_shndx != SHN_UNDEF) &&
            (symbol->st_shndx != SHN_ABS) &&
            (addr != ctx->init) &&
            (addr != ctx->retval->fini))
        {
            dbgprintf(("Exporting '%s' as '%p' ...\n", symstr, addr));
            if (!add_exported_symbol(ctx, symstr, addr))
                return 0;
            else if (ctx->retval->syms_count == symcount)
                break;  // found them all.
        } // if
    } // for

    assert(ctx->retval->syms_count == symcount);
    return 1;
} // build_export_list


static int call_so_init(ElfContext *ctx)
{
    if (ctx->init != NULL)
        ((ElfInitFn) ctx->init)(0, NULL, NULL);

    if (ctx->init_array != NULL)
    {
        int i;
        for (i = 0; i < ctx->init_array_count; i++)
            ((ElfInitFn) ctx->init_array[i])(0, NULL, NULL);
    } // if

    return 1;
} // call_so_init

static void *noop_loader(const char *soname, const char *rpath, const char *runpath) { return NULL; }
static void *noop_resolver(void *handle, const char *sym) { return NULL; }
static void noop_unloader(void *handle) {}

void *MOJOELF_dlopen_mem(const void *buf, const long buflen,
                         const MOJOELF_Callbacks *callbacks)
{
    static const MOJOELF_Callbacks nullcb = { NULL, NULL, NULL };
    ElfHandle *handle = NULL;
    ElfContext ctx;

    assert(sizeof (ElfHeader) == MOJOELF_SIZEOF_ELF_HEADER);
    assert(sizeof (ElfProgram) == MOJOELF_SIZEOF_PROGRAM_HEADER);
    assert(sizeof (ElfSection) == MOJOELF_SIZEOF_SECTION_HEADER);

    handle = (ElfHandle *) Malloc(sizeof (ElfHandle));
    if (handle == NULL)
        return NULL;

    if (callbacks == NULL)
        callbacks = &nullcb;

    Memzero(&ctx, sizeof (ElfContext));
    ctx.buf = (const uint8 *) buf;
    ctx.buflen = (size_t) buflen;
    ctx.loader = callbacks->loader ? callbacks->loader : noop_loader;
    ctx.resolver = callbacks->resolver ? callbacks->resolver : noop_resolver;
    ctx.unloader = callbacks->unloader ? callbacks->unloader : noop_unloader;
    ctx.header = (const ElfHeader *) buf;
    ctx.retval = handle;
    ctx.retval->mmapaddr = ((void *) MAP_FAILED);
    ctx.retval->entry = (void *) ctx.header->e_entry;
    ctx.retval->unloader = ctx.unloader;

    // here we go.
    if (!validate_elf_header(&ctx)) goto fail;
    else if (!process_program_headers(&ctx)) goto fail;
    else if (!map_pages(&ctx)) goto fail;
    else if (!walk_dynamic_table(&ctx)) goto fail;
    else if (!process_section_headers(&ctx)) goto fail;
    else if (!load_external_dependencies(&ctx)) goto fail;
    else if (!build_export_list(&ctx)) goto fail;
    else if (!fixup_relocations(&ctx)) goto fail;
    else if (!protect_pages(&ctx)) goto fail;
    else if (!call_so_init(&ctx)) goto fail;

    // we made it!
    return ctx.retval;

fail:
    ctx.retval->fini = NULL;  // don't try to call this in MOJOELF_dlclose()!
    MOJOELF_dlclose(ctx.retval);  // clean up any half-complete stuff.
    return NULL;
} // MOJOELF_dlopen_mem


void *MOJOELF_dlsym(void *lib, const char *sym)
{
    const ElfHandle *h = (const ElfHandle *) lib;
    int i;

    if (h == NULL)
    {
        set_dlerror("Bogus library handle");
        return NULL;
    } // if

    // !!! FIXME: can we hash these?
    for (i = 0; i < h->syms_count; i++)
    {
        if (Strcmp(h->syms[i].sym, sym) == 0)
            return h->syms[i].addr;
    } // for

    set_dlerror("Symbol not found");
    return NULL;
} // MOJOELF_dlsym


void MOJOELF_dlclose(void *lib)
{
    ElfHandle *h = (ElfHandle *) lib;
    int i;

    if (h == NULL)
        return;

    // ELF spec says FINI_ARRAY is executed in reverse order, so count down.
    if (h->fini_array != NULL)
    {
        for (i = h->fini_array_count-1; i >= 0; i--)
            ((ElfFiniFn) h->fini_array[i])();
    } // if

    if (h->fini != NULL)
        ((ElfFiniFn) h->fini)();

    if (h->dlopens != NULL)
    {
        for (i = 0; i < h->dlopens_count; i++)
        {
            if (h->dlopens[i])
                h->unloader(h->dlopens[i]);
        } // for
        free(h->dlopens);
    } // if

    if (h->mmapaddr != MAP_FAILED)
        munmap(h->mmapaddr, h->mmaplen);

    if (h->syms != NULL)
    {
        for (i = 0; i < h->syms_count; i++)
            free(h->syms[i].sym);
        free(h->syms);
    } // if

    free(h);
} // MOJOELF_dlclose


#if MOJOELF_SUPPORT_DLOPEN_FILE
void *MOJOELF_dlopen_file(const char *fname, const MOJOELF_Callbacks *cb)
{
    void *retval = NULL;
    struct stat statbuf;
    int fd = open(fname, O_RDONLY);
    uint8 *buf = NULL;

    if (fd == -1)
        set_dlerror(strerror(errno));
    else if (fstat(fd, &statbuf) == -1)
        set_dlerror(strerror(errno));
    else if ((buf = (uint8 *) malloc(statbuf.st_size)) == NULL)
        set_dlerror("out of memory");
    else if (read(fd, buf, statbuf.st_size) != statbuf.st_size)
        set_dlerror(strerror(errno));
    else
    {
        close(fd);
        fd = -1;
        retval = MOJOELF_dlopen_mem(buf, statbuf.st_size, cb);
    } // else

    if (fd != -1)
        close(fd);

    if (buf != NULL)
        free(buf);

    return retval;
} // MOJOELF_dlopen_file
#endif


const void *MOJOELF_getentry(void *lib)
{
    const ElfHandle *h = (const ElfHandle *) lib;
    return h ? h->entry : NULL;
} // MOJOELF_getentry

void MOJOELF_getmmaprange(void *lib, void **addr, unsigned long *len)
{
    const ElfHandle *h = (const ElfHandle *) lib;
    if (addr)
        *addr = h->mmapaddr;
    if (len)
        *len = (unsigned long) h->mmaplen;
} // MOJOELF_getmmaprange


// end of mojoelf.c ...

