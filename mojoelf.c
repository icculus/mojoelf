#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dlfcn.h>

// ELF specifications: http://refspecs.freestandards.org/elf/

#ifdef __cplusplus
extern "C" {
#endif
void *MOJOELF_dlopen_mem(const void *buf, const long buflen);
void *MOJOELF_dlopen_file(const char *fname);
void *MOJOELF_dlsym(void *lib, const char *sym);
void MOJOELF_dlclose(void *lib);
const char *MOJOELF_dlerror(void);
#ifdef __cplusplus
}
#endif


#define MOJOELF_TEST 1   // compiles a main() for a test app.
#define MOJOELF_SUPPORT_DLERROR 1
#define MOJOELF_SUPPORT_DLOPEN_FILE 1
//#define MOJOELF_ALLOW_SYSTEM_RESOLVE 1

typedef intptr_t intptr;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uintptr_t uintptr;

#if 1 //defined(__linux__)
    #define MOJOELF_OSABI 0  // ELFOSABI_SYSV ... not ELFOSABI_LINUX!
    #define MOJOELF_OSABIVERSION 0
#else
    #error Please define your platform.
#endif

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

#define ELF_ST_BIND(i) ((i) >> 4)

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

typedef struct ElfHandle  // this is what MOJOELF_dlopen_mem() returns.
{
    int mmaps_count;
    void *mmapaddr;
    size_t mmaplen;
    int syms_count;
    ElfSymbols *syms;
    void *fini;
    int dlopens_count;
    #if MOJOELF_ALLOW_SYSTEM_RESOLVE
    void **dlopens;
    #endif
} ElfHandle;


// Actual shared object loading happens here.

static inline void *Malloc(const size_t len)
{
    void *retval = calloc(1, len);
    if (retval == NULL)
        set_dlerror("Out of memory");
    return retval;
} // Malloc

static inline void *Realloc(void *ptr, const size_t len)
{
    void *retval = realloc(ptr, len);
    if (retval == NULL)
        set_dlerror("Out of memory");
    return retval;
} // Realloc

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
    size_t mmaplen;  // number of bytes we want to mmap().
    const char *strtab;  // string table for dynamic symbols.
    size_t strtablen;  // length in bytes of dynamic symbol string table.
    const ElfSymTable *symtab;  // the symbol table.
    const ElfDynTable *dyntabs[32];  // indexable pointers to dynamic tables.
} ElfContext;

#define DLOPEN_FAIL(err) do { set_dlerror(err); return 0; } while (0)

static int validate_elf_header(const ElfContext *ctx)
{
    const ElfHeader *hdr = ctx->header;
    const uint8 *buf = hdr->e_ident;
    if (ctx->buflen < 64)
        DLOPEN_FAIL("Not enough data");
    else if ((buf[0]!=0x7F) || (buf[1]!='E') || (buf[2]!='L') || (buf[3]!='F'))
        DLOPEN_FAIL("Not an ELF file");
    else if (buf[4] != MOJOELF_ELFCLASS)
        DLOPEN_FAIL("Unsupported/bogus ELF class");
    else if (buf[5] != MOJOELF_ELFDATA)
        DLOPEN_FAIL("Unsupported/bogus ELF data ordering");
    else if (buf[6] != 1)
        DLOPEN_FAIL("Unsupported/bogus ELF file version");
    else if (buf[7] != MOJOELF_OSABI)
        DLOPEN_FAIL("Unsupported/bogus ELF OSABI");
    else if (buf[8] != MOJOELF_OSABIVERSION)
        DLOPEN_FAIL("Unsupported/bogus ELF OSABI");
    else if (hdr->e_type != 3)  // ET_DYN (.so files)
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

        else if ((program->p_type == 1) && (program->p_memsz > 0))  // PT_LOAD
        {
            const size_t endaddr = program->p_vaddr + program->p_memsz;
            if (endaddr > ctx->mmaplen)
                ctx->mmaplen = endaddr;
            if (program->p_vaddr < ctx->base)
                ctx->base = program->p_vaddr;
        } // else if

        // When we see this header, take note for later.
        else if (program->p_type == 2)  // PT_DYNAMIC
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


// Get the ELF programs into memory at the right place and set protections.
static int map_pages(ElfContext *ctx)
{
    // Get us an aligned block of memory where we can change page permissions.
    // This is big enough to store all the program blocks and place them at
    //  the correct relative offsets.
    const size_t offset = (size_t) ctx->header->e_phoff;
    const ElfProgram *program = (const ElfProgram *) (ctx->buf + offset);
    const int header_count = (int) ctx->header->e_phnum;
    const size_t mmaplen = ctx->mmaplen;
    const int mmapprot = PROT_READ | PROT_WRITE;
    const int mmapflags = MAP_ANON | MAP_PRIVATE;
    void *mmapaddr = mmap(NULL, mmaplen, mmapprot, mmapflags, -1, 0);
    int i;

    if (mmapaddr == ((void *) MAP_FAILED))
        DLOPEN_FAIL("mmap failed");

    memset(mmapaddr, '\0', mmaplen);
    ctx->retval->mmapaddr = mmapaddr;
    ctx->retval->mmaplen = mmaplen;

    // Put the program blocks at the correct relative positions.
    for (i = 0; i < header_count; i++, program++)
    {
        if ((program->p_type == 1) && (program->p_memsz > 0))  // PT_LOAD
        {
            uint8 *ptr = ((uint8 *) mmapaddr) + (program->p_vaddr - ctx->base);
            const size_t len = (const size_t) program->p_memsz;
            const int prot = ((program->p_flags & 1) ? PROT_EXEC : 0)  |
                             ((program->p_flags & 2) ? PROT_WRITE : 0) |
                             ((program->p_flags & 4) ? PROT_READ : 0)  ;
            memcpy(ptr, ctx->buf + program->p_offset, program->p_filesz);
            if ((prot != mmapprot) && (mprotect(ptr, len, prot) == -1))
                DLOPEN_FAIL("mprotect failed");
        } // if
    } // for

    return 1;
} // map_pages


static int walk_dynamic_table(ElfContext *ctx)
{
    // preliminary walkthrough of the dynamic table.
    const ElfDynTable *dyntab = ctx->dyntab;
    const int dyntabcount = ctx->dyntabcount;
    uint8 *mmapaddr = (uint8 *) ctx->retval->mmapaddr;
    int i;

    for (i = 0; i < dyntabcount; i++, dyntab++)
    {
        const int tag = (int) dyntab->d_tag;

        if (tag == 0)  // DT_NULL
            continue;
        else if (tag == 1)  // DT_NEEDED
        {
            ctx->retval->dlopens_count++;
            continue;
        } // if

        if (tag < (sizeof (ctx->dyntabs) / sizeof (ctx->dyntabs[0])))
        {
            if (ctx->dyntabs[tag] != NULL)
                DLOPEN_FAIL("Illegal duplicate dynamic tables");
            ctx->dyntabs[tag] = dyntab;
        } // if
    } // for

    // okay, all the tables are indexed now, so we don't have to worry about
    //  dupes or missing fields, or ordering issues.
    // Now that we've shuffled this stuff into an index, process it.

    if (ctx->dyntabs[5] == NULL)  // DT_STRTAB
        DLOPEN_FAIL("No DT_STRTAB table");
    else if (ctx->dyntabs[10] == NULL)  // DT_STRSZ
        DLOPEN_FAIL("No DT_STRSZ table");
    else if (ctx->dyntabs[6] == NULL)  // DT_SYMTAB
        DLOPEN_FAIL("No DT_SYMTAB table");
    else if (ctx->dyntabs[11] == NULL)  // DT_SYMENT
        DLOPEN_FAIL("No DT_SYMENT table");
    else if (ctx->dyntabs[11]->d_un.d_val != MOJOELF_SIZEOF_SYMENT) // DT_SYMENT
        DLOPEN_FAIL("Bogus DT_SYMENT value");

    // !!! FIXME: is there a size of the symbol table without parsing
    // !!! FIXME:  the section headers?
    if (ctx->dyntabs[6]->d_un.d_ptr >= ctx->buflen)  // DT_SYMTAB
        DLOPEN_FAIL("Bogus DT_SYMTAB offset");
    ctx->symtab = (const ElfSymTable *) (ctx->buf+ctx->dyntabs[6]->d_un.d_ptr);

    if (ctx->dyntabs[7])  // DT_RELA
    {
        if (ctx->dyntabs[8] == NULL)  // DT_RELASZ
            DLOPEN_FAIL("No DT_RELASZ table");
        else if (ctx->dyntabs[9] == NULL)  // DT_RELAENT
            DLOPEN_FAIL("No DT_RELAENT table");
        else if (ctx->dyntabs[9]->d_un.d_val != MOJOELF_SIZEOF_RELAENT)
            DLOPEN_FAIL("Unsupported/bogus DT_RELAENT value");
        else if (ctx->dyntabs[8]->d_un.d_val % MOJOELF_SIZEOF_RELAENT)
            DLOPEN_FAIL("Bogus DT_RELASZ value");
        else if (ctx->dyntabs[7]->d_un.d_ptr +
                 ctx->dyntabs[8]->d_un.d_val >= ctx->buflen)
            DLOPEN_FAIL("Bogus DT_RELA value");
    } // if

    if (ctx->dyntabs[17])  // DT_REL
    {
        if (ctx->dyntabs[18] == NULL)  // DT_RELSZ
            DLOPEN_FAIL("No DT_RELSZ table");
        else if (ctx->dyntabs[19] == NULL)  // DT_RELENT
            DLOPEN_FAIL("No DT_RELENT table");
        else if (ctx->dyntabs[19]->d_un.d_val != MOJOELF_SIZEOF_RELENT)
            DLOPEN_FAIL("Unsupported/bogus DT_RELENT value");
        else if (ctx->dyntabs[18]->d_un.d_val % MOJOELF_SIZEOF_RELENT)
            DLOPEN_FAIL("Bogus DT_RELSZ value");
        else if (ctx->dyntabs[17]->d_un.d_ptr +
                 ctx->dyntabs[18]->d_un.d_val >= ctx->buflen)
            DLOPEN_FAIL("Bogus DT_REL value");
    } // if

    if (ctx->dyntabs[23])  // DT_JMPREL
    {
        if (ctx->dyntabs[20] == NULL)  // DT_PLTREL
            DLOPEN_FAIL("No DT_PLTREL table");
        else if (ctx->dyntabs[2] == NULL)  // DT_PLTRELSZ
            DLOPEN_FAIL("No DT_PLTRELSZ table");

        if (ctx->dyntabs[20]->d_un.d_val == 17)  // DT_PLTREL -> DT_REL
        {
            if (ctx->dyntabs[19] == NULL)  // DT_RELENT
                DLOPEN_FAIL("No DT_RELENT table");
            else if (ctx->dyntabs[19]->d_un.d_val != MOJOELF_SIZEOF_RELENT)
                DLOPEN_FAIL("Unsupported/bogus DT_RELENT value");
            else if (ctx->dyntabs[2]->d_un.d_val % MOJOELF_SIZEOF_RELENT)
                DLOPEN_FAIL("Bogus DT_PLTRELSZ value");
            else if (ctx->dyntabs[17]->d_un.d_ptr +
                     ctx->dyntabs[18]->d_un.d_val >= ctx->buflen)
                DLOPEN_FAIL("Bogus DT_PLTREL value");
        } // if

        else if (ctx->dyntabs[20]->d_un.d_val == 7)  // DT_PLTREL -> DT_RELA
        {
            if (ctx->dyntabs[9] == NULL)  // DT_RELAENT
                DLOPEN_FAIL("No DT_RELAENT table");
            else if (ctx->dyntabs[9]->d_un.d_val != MOJOELF_SIZEOF_RELAENT)
                DLOPEN_FAIL("Unsupported/bogus DT_RELAENT value");
            else if (ctx->dyntabs[2]->d_un.d_val % MOJOELF_SIZEOF_RELAENT)
                DLOPEN_FAIL("Bogus DT_PLTRELSZ value");
            else if (ctx->dyntabs[7]->d_un.d_ptr +
                     ctx->dyntabs[8]->d_un.d_val >= ctx->buflen)
                DLOPEN_FAIL("Bogus DT_PLTREL value");
        } // else if

        else
        {
            DLOPEN_FAIL("Bogus DT_PLTREL value");
        } // else
    } // if

    ctx->strtablen = (size_t) ctx->dyntabs[10]->d_un.d_val;  // DT_STRSZ
    ctx->strtab = (const char *) (ctx->buf + ctx->dyntabs[5]->d_un.d_ptr);
    if (ctx->strtablen == 0)  // technically this is allowed, but oh well.
        DLOPEN_FAIL("Dynamic string table is empty");
    else if ((ctx->dyntabs[5]->d_un.d_ptr + ctx->strtablen) > ctx->buflen)
        DLOPEN_FAIL("Dynamic string table has bogus offset and/or length");
    else if (ctx->strtab[0] != '\0')
        DLOPEN_FAIL("Dynamic string table doesn't start with null byte");
    else if (ctx->strtab[ctx->strtablen - 1] != '\0')
        DLOPEN_FAIL("Dynamic string table doesn't end with null byte");

    if (ctx->dyntabs[12])  // DT_INIT
        ctx->init = mmapaddr + (ctx->dyntabs[12]->d_un.d_ptr-ctx->base);

    if (ctx->dyntabs[13])  // DT_FINI
        ctx->retval->fini = mmapaddr + (ctx->dyntabs[13]->d_un.d_ptr-ctx->base);

    if (ctx->dyntabs[25])  // DT_INIT_ARRAY
        DLOPEN_FAIL("write me");
        //if (ctx->dyntabs[27])  // DT_INIT_ARRAYSZ
    if (ctx->dyntabs[26])  // DT_FINI_ARRAY
        DLOPEN_FAIL("write me");
        //if (ctx->dyntabs[28])  // DT_FINI_ARRAYSZ


    return 1;
} // walk_dynamic_table


// We might need to load symbols in other libraries. Comically, we
//  use dlopen() and dlsym() for this, but you can forbid this, in
//  case you know all the external libraries you want are already
//  loaded and you can supply all the symbols yourself.
static int load_external_dependencies(ElfContext *ctx)
{
#if MOJOELF_ALLOW_SYSTEM_RESOLVE
    const int dlopens_count = ctx->retval->dlopens_count;
    const ElfDynTable *dyntab = ctx->dyntab;
    const int dyntabcount = ctx->dyntabcount;
    int i;

    if (dlopens_count == 0)
        return 1;  // nothing to do.

    // !!! FIXME: We currently can't give accurate results if there's an rpath.
    if (ctx->dyntabs[15] != NULL)  // DT_RPATH
        DLOPEN_FAIL("DT_RPATH isn't supported at the moment");

    ctx->retval->dlopens_count = 0;
    ctx->retval->dlopens = (void **) Malloc(dlopens_count * sizeof (void *));
    if (ctx->retval->dlopens == NULL)
        return 0;

    // Find the libraries to load.
    for (i = 0; i < dyntabcount; i++, dyntab++)
    {
        if (dyntab->d_tag == 1)  // DT_NEEDED
        {
            const size_t offset = dyntab->d_un.d_val;
            if (offset >= ctx->strtablen)
                DLOPEN_FAIL("Bogus DT_NEEDED string");
            else
            {
                const char *str = ctx->strtab + offset;
                void *lib = dlopen(str, RTLD_NOW);
                if (lib == NULL)
                    DLOPEN_FAIL("Couldn't load dependency");
                ctx->retval->dlopens[ctx->retval->dlopens_count++] = lib;
                if (ctx->retval->dlopens_count == dlopens_count)
                    return 1;  // done!
            } // else
        } // if
    } // for

    assert(0);
    DLOPEN_FAIL("Bug: didn't see as many DT_NEEDED on second pass");
#endif

    return 1;
} // load_external_dependencies


static int add_exported_symbol(ElfContext *ctx, const char *sym, void *addr)
{
    // !!! FIXME: We should really use a hash table for all this.
    const int syms_count = ctx->retval->syms_count;
    ElfSymbols *syms = ctx->retval->syms;
    void *ptr;
    int i;

    for (i = 0; i < syms_count; i++)
    {
        if (strcmp(syms[i].sym, sym) == 0)
        {
            // !!! FIXME: This failure is possibly our bug, since we don't
            // !!! FIXME:  parse the symbol table directly.
            if (addr != syms[i].addr)
                DLOPEN_FAIL("Exporting symbol twice with different addresses");
            return 1;  // already added.
        } // if
    } // for

    ptr = Realloc(syms, (syms_count + 1) * sizeof (ElfSymbols));
    if (ptr == NULL)
        return 0;
    syms = ctx->retval->syms = (ElfSymbols *) ptr;

    syms[syms_count].sym = (char *) Malloc(strlen(sym) + 1);
    if (syms[syms_count].sym == NULL)
        return 0;

    strcpy(syms[syms_count].sym, sym);
    syms[syms_count].addr = addr;
    ctx->retval->syms_count++;

    return 1;
} // add_exported_symbol


static int resolve_symbol(ElfContext *ctx, const uint32 sym, uintptr *_addr)
{
    // !!! FIXME: figure out symbol table count, fail if sym >= to it.
    const ElfSymTable *symbol = ctx->symtab + sym;
    const char *symstr = NULL;
    void *addr = ((uint8 *) ctx->retval->mmapaddr) + symbol->st_value;

    if (symbol->st_name >= ctx->strtablen)
        DLOPEN_FAIL("Bogus symbol name");

    symstr = ctx->strtab + symbol->st_name;

    if ((sym == 0) || (*symstr == '\0'))
        /* no-op */;  // we're done already.

    else if (symbol->st_shndx == 0)  // SHN_UNDEF
    {
        printf("Resolving '%s' ...\n", symstr);

        #if MOJOELF_ALLOW_SYSTEM_RESOLVE
        addr = dlsym(NULL, symstr);
        int i;
        for (i = 0; (addr == NULL) && (i < ctx->retval->dlopens_count); i++)
            addr = dlsym(ctx->retval->dlopens[i], symstr);
        #endif

        // !!! FIXME: let app supply symbol, too.

        printf("Resolved '%s' to %p ...\n", symstr, addr);

        if (addr == NULL)
        {
            if (ELF_ST_BIND(symbol->st_info) != 2)  // STB_WEAK
                DLOPEN_FAIL("Couldn't resolve symbol");
        } // if
    } // if

    else  // This is a symbol we export, add it to our list.
    {
        printf("Exporting '%s' as '%p' ...\n", symstr, addr);
        add_exported_symbol(ctx, symstr, (void *) symbol->st_value);
    } // else

    *_addr = (uintptr) addr;
    return 1;
} // resolve_sym

static int do_fixup(ElfContext *ctx, const uint32 r_type, const uint32 r_sym,
                    const uintptr r_offset, const intptr r_addend)
{
    uint8 *mmapaddr = (uint8 *) ctx->retval->mmapaddr;
    uintptr *fixup = (uintptr *) (mmapaddr + (r_offset - ctx->base));
    uintptr addr = 0;

    if (!resolve_symbol(ctx, r_sym, &addr))
        return 0;

    switch (r_type)
    {
        // There are way more than these, but these seem to be the
        //  only ones average libraries use.
        // Note libc.so.6 itself also seems to use: R_*_64, R_*_TPOFF64
        case 6:  // R_[X86_64|386]_GLOB_DATA
        case 7:  // R_[X86_64|386]_JUMP_SLOT
            *fixup = addr;
            break;
        case 8:  // R_[X86_64|386]_RELATIVE
            *fixup = (uintptr) (addr + r_addend);
            break;

        case 0:  // R_[X86_64|386]_NONE
            break;  // do nothing.

        default:
            DLOPEN_FAIL("write me");  // haven't seen this yet.
            break;
    } // switch

    return 1;
} // do_fixup

static int fixup_rela_internal(ElfContext *ctx, const ElfDynTable *dt_rela)
{
    const size_t offset = (size_t) dt_rela->d_un.d_ptr;
    const size_t relasz = (size_t) ctx->dyntabs[8]->d_un.d_val;  // DT_RELASZ
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
    const ElfDynTable *dt_rela = ctx->dyntabs[7];  // DT_RELA
    if (dt_rela == NULL)  // it's optional, we're done if it's not there.
        return 1;
    return fixup_rela_internal(ctx, dt_rela);
} // fixup_rela

static int fixup_rel_internal(ElfContext *ctx, const ElfDynTable *dt_rel)
{
    const size_t offset = (size_t) dt_rel->d_un.d_ptr;
    const size_t relsz = (size_t) ctx->dyntabs[18]->d_un.d_val;  // DT_RELSZ
    const size_t count = relsz / MOJOELF_SIZEOF_RELENT;
    const ElfRelA *rel = (const ElfRelA *) (ctx->buf + offset);
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
    const ElfDynTable *dt_rel = ctx->dyntabs[17];  // DT_RELA
    if (dt_rel == NULL)  // it's optional, we're done if it's not there.
        return 1;
    return fixup_rel_internal(ctx, dt_rel);
} // fixup_rel

static inline int fixup_jmprel(ElfContext *ctx)
{
    const ElfDynTable *dt_jmprel = ctx->dyntabs[23];  // DT_JMPREL
    if (dt_jmprel == NULL)  // it's optional, we're done if it's not there.
        return 1;

    if (ctx->dyntabs[20]->d_un.d_val == 7) // DT_RELA
        return fixup_rela_internal(ctx, dt_jmprel);

    assert(ctx->dyntabs[20]->d_un.d_val == 17); // DT_REL
    return fixup_rel_internal(ctx, dt_jmprel);
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


static int call_so_init(ElfContext *ctx)
{
    if (ctx->init != NULL)
        ((ElfInitFn) ctx->init)(0, NULL, NULL);

    // !!! FIXME: handle DT_INIT_ARRAYs

    return 1;
} // call_so_init


void *MOJOELF_dlopen_mem(const void *buf, const long buflen)
{
    ElfContext ctx;

    assert(sizeof (ElfHeader) == MOJOELF_SIZEOF_ELF_HEADER);
    assert(sizeof (ElfProgram) == MOJOELF_SIZEOF_PROGRAM_HEADER);
    assert(sizeof (ElfSection) == MOJOELF_SIZEOF_SECTION_HEADER);

    memset(&ctx, '\0', sizeof (ElfContext));
    ctx.buf = (const uint8 *) buf;
    ctx.buflen = (size_t) buflen;
    ctx.header = (const ElfHeader *) buf;
    ctx.retval = (ElfHandle *) Malloc(sizeof (ElfHandle));
    if (ctx.retval == NULL)
        goto fail;
    ctx.retval->mmapaddr = ((void *) MAP_FAILED);

    // here we go.
    if (!validate_elf_header(&ctx)) goto fail;
    else if (!process_program_headers(&ctx)) goto fail;
    else if (!map_pages(&ctx)) goto fail;
    else if (!walk_dynamic_table(&ctx)) goto fail;
    else if (!load_external_dependencies(&ctx)) goto fail;
    else if (!fixup_relocations(&ctx)) goto fail;
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
        if (strcmp(h->syms[i].sym, sym) == 0)
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

    if (h->fini != NULL)
        ((ElfFiniFn) h->fini)();

    // !!! FIXME: handle DT_FINI_ARRAYs

    #if MOJOELF_ALLOW_SYSTEM_RESOLVE
    if (h->dlopens != NULL)
    {
        for (i = 0; i < h->dlopens_count; i++)
        {
            if (h->dlopens[i])
                dlclose(h->dlopens[i]);
        } // for
        free(h->dlopens);
    } // if
    #endif

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


#if MOJOELF_SUPPORT_DLOPEN_FILE || MOJOELF_TEST
void *MOJOELF_dlopen_file(const char *fname)
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
        retval = MOJOELF_dlopen_mem(buf, statbuf.st_size);
    } // else

    if (fd != -1)
        close(fd);

    if (buf != NULL)
        free(buf);

    return retval;
} // MOJOELF_dlopen_file
#endif


#if MOJOELF_TEST
int main(int argc, char **argv)
{
    void *lib = NULL;
    int i;
    for (i = 1; i < argc; i++)
    {
        lib = MOJOELF_dlopen_file(argv[i]);
        if (lib == NULL)
            printf("failed '%s'! (%s)\n", argv[i], MOJOELF_dlerror());
        else
        {
            printf("loaded '%s'!\n", argv[i]);
            MOJOELF_dlclose(lib);
        } // else
    } // for

    return 0;
} // main
#endif

// end of mojoelf.c ...

