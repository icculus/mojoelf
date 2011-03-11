#ifndef INCL_MOJOELF_H
#define INCL_MOJOELF_H

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

#endif

/* end of mojoelf.h ... */


