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

#define MOJOELF_SUPPORT_DLERROR 1
#define MOJOELF_SUPPORT_DLOPEN_FILE 1
#define MOJOELF_ALLOW_SYSTEM_RESOLVE 1

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
#elif MOJOELF_64BIT
    #define MOJOELF_PAGESIZE 4096
    #define MOJOELF_ELFCLASS 2  // ELFCLASS64
    #define MOJOELF_SIZEOF_ELF_HEADER 64
    #define MOJOELF_SIZEOF_PROGRAM_HEADER 56
    #define MOJOELF_SIZEOF_SECTION_HEADER 64
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

#define VALIDATE_FAIL(err) do { set_dlerror(err); return 0; } while (0)
static int validate_elf_program(const ElfProgram *hdr, const size_t bufsize)
{
    if ((hdr->p_offset + hdr->p_filesz) > bufsize)
        VALIDATE_FAIL("Bogus ELF program offset/size");
    else if (hdr->p_filesz > hdr->p_memsz)
        VALIDATE_FAIL("Bogus ELF program size");
    return 1;
} // validate_elf_program

static int validate_elf_section(const ElfSection *hdr, const size_t bufsize,
                                const uintptr maxstr)
{
    if (hdr->sh_offset + hdr->sh_size > bufsize)
        VALIDATE_FAIL("Bogus ELF program offset/size");
    else if ((maxstr > 0) && (hdr->sh_name > maxstr))
        VALIDATE_FAIL("Bogus ELF section name index");
    return 1;
} // validate_elf_section

static int validate_elf_header(const ElfHeader *hdr, const size_t bufsize)
{
    const uint8 *buf = hdr->e_ident;
    if (bufsize < 64)
        VALIDATE_FAIL("Not enough data");
    else if ((buf[0]!=0x7F) || (buf[1]!='E') || (buf[2]!='L') || (buf[3]!='F'))
        VALIDATE_FAIL("Not an ELF file");
    else if (buf[4] != MOJOELF_ELFCLASS)
        VALIDATE_FAIL("Unsupported/bogus ELF class");
    else if (buf[5] != MOJOELF_ELFDATA)
        VALIDATE_FAIL("Unsupported/bogus ELF data ordering");
    else if (buf[6] != 1)
        VALIDATE_FAIL("Unsupported/bogus ELF file version");
    else if (buf[7] != MOJOELF_OSABI)
        VALIDATE_FAIL("Unsupported/bogus ELF OSABI");
    else if (buf[8] != MOJOELF_OSABIVERSION)
        VALIDATE_FAIL("Unsupported/bogus ELF OSABI");
    else if (hdr->e_type != 3)  // ET_DYN (.so files)
        VALIDATE_FAIL("Unsupported/bogus ELF object type");
    else if (hdr->e_machine != MOJOELF_MACHINE_TYPE)
        VALIDATE_FAIL("Unsupported/bogus ELF machine type");
    else if (hdr->e_version != 1)  // "current"
        VALIDATE_FAIL("Unsupported/bogus ELF object version");
    else if (hdr->e_ehsize != sizeof (ElfHeader))
        VALIDATE_FAIL("Unsupported/bogus ELF main header size");
    else if (hdr->e_phentsize != sizeof (ElfProgram))
        VALIDATE_FAIL("Unsupported/bogus ELF program header size");
    else if (hdr->e_shentsize != sizeof (ElfSection))
        VALIDATE_FAIL("Unsupported/bogus ELF section header size");
    else if ((hdr->e_phoff + (hdr->e_phnum * hdr->e_phentsize)) > bufsize)
        VALIDATE_FAIL("Bogus ELF program header offset/count");
    else if ((hdr->e_shoff + (hdr->e_shnum * hdr->e_shentsize)) > bufsize)
        VALIDATE_FAIL("Bogus ELF section header offset/count");

    if (hdr->e_shstrndx != 0)
    {
        if (hdr->e_shstrndx >= hdr->e_shnum)
            VALIDATE_FAIL("Bogus ELF section header string table index");
        else
        {
            const uint8 *ptr = ((const uint8 *) hdr) + hdr->e_shoff;
            const ElfSection *table = ((const ElfSection*)ptr)+hdr->e_shstrndx;
            if (!validate_elf_section(table, bufsize, 0))
                return 0;  // it will have set dlerror.

            if (table->sh_type != 3)  // SHT_STRTAB
                VALIDATE_FAIL("String table section has wrong type");

            if (table->sh_size > 0)  // zero is legal, apparently.
            {
                ptr = ((const uint8 *) hdr) + table->sh_offset;
                if (ptr[0] != '\0')
                    VALIDATE_FAIL("String table doesn't start with null byte");
                else if (ptr[table->sh_size - 1] != '\0')
                    VALIDATE_FAIL("String table doesn't end with null byte");
            } // if
        } // if
    } // if

    return 1;  // all good!
} // validate_elf_header

typedef struct ElfSymbols
{
    char *sym;
    void *addr;
} ElfSymbols;

typedef struct ElfHandle
{
    int mmaps_count;
    void *mmapaddr;
    size_t mmaplen;
    int syms_count;
    ElfSymbols *syms;
    void *fini;
    #if MOJOELF_ALLOW_SYSTEM_RESOLVE
    int dlopens_count;
    void **dlopens;
    #endif
} ElfHandle;

void MOJOELF_dlclose(void *lib)
{
    ElfHandle *h = (ElfHandle *) lib;
    int i;

    if (h == NULL)
        return;

    // !!! FIXME: call h->fini().

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

void *MOJOELF_dlsym(void *lib, const char *sym)
{
    const ElfHandle *h = (const ElfHandle *) lib;
    int i;

    if (h == NULL)
    {
        set_dlerror("Bogus library handle");
        return NULL;
    } // if

    for (i = 0; i < h->syms_count; i++)
    {
        if (strcmp(h->syms[i].sym, sym) == 0)
            return h->syms[i].addr;
    } // for

    set_dlerror("Symbol not found");
    return NULL;
} // MOJOELF_dlsym

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

void *MOJOELF_dlopen_mem(const uint8 *buf, const size_t buflen)
{
    #define DLOPEN_FAIL(err) do { set_dlerror(err); goto fail; } while (0)
    ElfHandle *retval = NULL;
    const int mmapprot = PROT_READ | PROT_WRITE;
    const int mmapflags = MAP_ANON | MAP_PRIVATE;
    const ElfHeader *header = (const ElfHeader *) buf;
    const ElfProgram *program = NULL;
    const ElfSection *section = NULL;
    const ElfProgram *dyntabprog = NULL;
    const ElfDynTable *dyntab = NULL;
    const char *strtab = NULL;
    const char *dynstrtab = NULL;
    void *init = NULL;
    uintptr base = ((uintptr) -1);
    size_t mmaplen = 0;
    int symtabidx = -1;
    uintptr maxstr = 0;
    uint32 i = 0;

    assert(sizeof (ElfHeader) == MOJOELF_SIZEOF_ELF_HEADER);
    assert(sizeof (ElfProgram) == MOJOELF_SIZEOF_PROGRAM_HEADER);
    assert(sizeof (ElfSection) == MOJOELF_SIZEOF_SECTION_HEADER);

    if (!validate_elf_header(header, buflen))
        goto fail;

    // set up the string table right away, now that it's safe.
    section = ((const ElfSection *)(buf+header->e_shoff)) + header->e_shstrndx;
    maxstr = section->sh_size - 1;
    strtab = ((const char *) buf) + section->sh_offset;

    retval = (ElfHandle *) Malloc(sizeof (ElfHandle));
    if (retval == NULL)
        goto fail;
    retval->mmapaddr = MAP_FAILED;

    // Figure out the memory range we'll need to allocate.
    program = (const ElfProgram *) (buf + header->e_phoff);
    for (i = 0; i < header->e_phnum; i++, program++)
    {
        if (!validate_elf_program(program, buflen))
            goto fail;
        else if ((program->p_type == 1) && (program->p_memsz > 0))  // PT_LOAD
        {
            const size_t endaddr = program->p_vaddr + program->p_memsz;
            if (endaddr > mmaplen)
                mmaplen = endaddr;
            if (program->p_vaddr < base)
                base = program->p_vaddr;
        } // else if
        else if (program->p_type == 2)  // PT_DYNAMIC
        {
            if (dyntabprog != NULL)  // Can there be more than one of these?!
                DLOPEN_FAIL("Multiple PT_DYNAMIC tables");
            dyntabprog = ((const ElfProgram *) (buf + header->e_phoff)) + i;
        } // else if
    } // for

    if (mmaplen == 0)
        DLOPEN_FAIL("No loadable pages");
    else if (dyntabprog == NULL)
        DLOPEN_FAIL("No PT_DYNAMIC table");

    // Calculate the final base address and allocation size.
    base -= (base % MOJOELF_PAGESIZE);
    mmaplen -= (size_t) base;
    mmaplen += (MOJOELF_PAGESIZE - (mmaplen % MOJOELF_PAGESIZE));

    // Get us an aligned block of memory where we can change page permissions.
    // This is big enough to store all the program blocks and place them at
    //  the correct relative offsets.
    retval->mmapaddr = mmap(NULL, mmaplen, mmapprot, mmapflags, -1, 0);

    if (retval->mmapaddr == MAP_FAILED)
        DLOPEN_FAIL("mmap failed");
    retval->mmaplen = mmaplen;
    memset(retval->mmapaddr, '\0', mmaplen);

    // Put the program blocks at the correct relative positions.
    program = (const ElfProgram *) (buf + header->e_phoff);
    for (i = 0; i < header->e_phnum; i++, program++)
    {
        if ((program->p_type == 1) && (program->p_memsz > 0))  // PT_LOAD
        {
            uint8 *ptr = ((uint8*) retval->mmapaddr) + (program->p_vaddr-base);
            const size_t len = (const size_t) program->p_memsz;
            const int prot = ((program->p_flags & 1) ? PROT_EXEC : 0)  |
                             ((program->p_flags & 2) ? PROT_WRITE : 0) |
                             ((program->p_flags & 4) ? PROT_READ : 0)  ;
            memcpy(ptr, buf + program->p_offset, program->p_filesz);
            if ((prot != mmapprot) && (mprotect(ptr, len, prot) == -1))
                DLOPEN_FAIL("mprotect failed");
        } // if
    } // for

#if 0
    // Okay, let's sort out some ELF sections now...
    section = (const ElfSection *) (buf + header->e_shoff);
    for (i = 0; i < header->e_shnum; i++, section++)
    {
        const char *name = strtab + section->sh_name;
        if (!validate_elf_section(section, buflen, maxstr))
            goto fail;

        if (section->sh_type == 11)  // SHT_DYNSYM
        {
            if (symtab != -1)  // Spec says only one (but that might change)
                DLOPEN_FAIL("Multiple dynamic symbol tables");
            symtab = i;
        } // if

        //if (strcmp(name, "") == 0)
    } // for
#endif

    // preliminary walkthrough of the dynamic table.
    #define MAX_DYNTABS 24
    const ElfDynTable *dyntabs[MAX_DYNTABS];
    memset(dyntabs, '\0', sizeof (dyntabs));

    dyntab = (const ElfDynTable *) (buf + dyntabprog->p_offset);
    for (i = 0; i < (dyntabprog->p_filesz / sizeof (*dyntab)); i++, dyntab++)
    {
        const int tag = (int) dyntab->d_tag;

        if (tag == 0)  // DT_NULL
            continue;
        else if (tag == 1)  // DT_NEEDED
        {
            retval->dlopens_count++;
            continue;
        } // if

        if (tag < MAX_DYNTABS)
        {
            if (dyntabs[tag] != NULL)
                DLOPEN_FAIL("Illegal duplicate dynamic table entry");
            dyntabs[tag] = dyntab;
        } // if
    } // for

    // Now that we've shuffled this stuff into an index, process it.
    if (dyntabs[5] == NULL)  // DT_STRTAB
        DLOPEN_FAIL("No dynamic string table");
    else if (dyntabs[10] == NULL)  // DT_STRSZ
        DLOPEN_FAIL("No dynamic string table size");

    dynstrtab = (const char *) (buf + dyntabs[5]->d_un.d_ptr);
    if (dynstrtab[0] != '\0')
        DLOPEN_FAIL("Dynstrtab doesn't start with null byte");
    // !!! FIXME: fails if d_val == 0
    //else if (dynstrtab[dyntab->d_un.d_val - 1] != '\0')
    //    DLOPEN_FAIL("Dynstrtab doesn't end with null byte");

    if (dyntabs[12])  // DT_INIT
        init = (void *) (buf + dyntabs[12]->d_un.d_ptr);

    if (dyntabs[13])  // DT_FINI
        retval->fini = (void *) (buf + dyntabs[13]->d_un.d_ptr);

    // We might need to load symbols in other libraries. Comically, we
    //  use dlopen() and dlsym() for this, but you can forbid this, in
    //  case you know all the external libraries you want are already
    //  loaded and you can supply all the symbols yourself.
    #if MOJOELF_ALLOW_SYSTEM_RESOLVE
    if (retval->dlopens_count > 0)
    {
        retval->dlopens = (void**) Malloc(retval->dlopens_count*sizeof(void*));
        if (retval->dlopens == NULL)
            goto fail;
        retval->dlopens_count = 0;
    } // if
    #endif

    // okay, round two on dynamic table: do heavy lifting.
    dyntab = (const ElfDynTable *) (buf + dyntabprog->p_offset);
    for (i = 0; i < (dyntabprog->p_filesz / sizeof (*dyntab)); i++, dyntab++)
    {
        const char *str;
        switch (dyntab->d_tag)
        {
            #if MOJOELF_ALLOW_SYSTEM_RESOLVE
            case 1:  // DT_NEEDED
                // !!! FIXME: DT_RPATH?
                str = dynstrtab + dyntab->d_un.d_val;
                retval->dlopens[retval->dlopens_count] = dlopen(str, RTLD_NOW);
                if (retval->dlopens[retval->dlopens_count] == NULL)
                    DLOPEN_FAIL("Couldn't load dependency");
                retval->dlopens_count++;
                break;
            #endif
        } // switch
    } // for

    // now it's time for relocation...


DLOPEN_FAIL("write me");

    if (init != NULL)
        DLOPEN_FAIL("fixme: call init()");

    return retval;

fail:
    retval->fini = NULL;  // don't ever try to call this in MOJOELF_dlclose()!
    MOJOELF_dlclose(retval);  // this will clean up any half-complete stuff.
    return NULL;
} // MOJOELF_dlopen_mem


#if MOJOELF_SUPPORT_DLOPEN_FILE
void *MOJOELF_dlopen(const char *fname)
{
    void *retval = NULL;
    struct stat statbuf;
    int fd = open(fname, O_RDONLY);
    char *buf = NULL;

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
} // MOJOELF_dlopen
#endif


int main(int argc, char **argv)
{
    void *lib = NULL;
    int i;
    for (i = 1; i < argc; i++)
    {
        lib = MOJOELF_dlopen(argv[i]);
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

// end of mojoelf.c ...

