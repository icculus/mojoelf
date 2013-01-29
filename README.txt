/**
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

MojoELF README.

To use this nonsense:

- Add mojoelf.c to your build.
- Compile with these #defines, or change the top of mojoelf.c:
    MOJOELF_SUPPORT_DLERROR 0  // remove MOJOELF_dlerror() + lots of strings.
    MOJOELF_SUPPORT_DLOPEN_FILE 0 // remove MOJOELF_dlopen_file()
    MOJOELF_REDUCE_LIBC_DEPENDENCIES 0  // use less libc calls. Scary!
    NDEBUG 1  // Turns off assert, which removes libc dependencies.
- Your calling code should #include mojoelf.h ...
- Put your ELF library in memory, and call MOJOELF_dlopen_mem() with the
  address of the memory buffer, the size of the buffer, and (optionally),
  callbacks that handle symbol resolution (they can be NULL). Details on
  the resolver callbacks are at the end of this document.
- If MOJOELF_dlopen_mem() returns non-NULL, the library is ready to use. If
  it returns NULL, there was a problem (MOJOELF_dlerror() will give you a
  human-readable error message). On success, you can free your buffer; we
  don't need it after MOJOELF_dlopen_mem() returns.
- MOJOELF_dlopen_file() does the same thing, but takes a filename instead of
  a memory buffer. Internally, it just loads the file into a malloc()'d
  buffer and calls MOJOELF_dlopen_mem().
- To request entry points into the library, use MOJOELF_dlsym():

      int (*my_function)(int argument) = MOJOELF_dlsym(lib, "AwesomeFunc");
      if (my_function == NULL)
          printf("couldn't find AwesomeFunc!\n");
      else
          printf("AwesomeFunc() returns %d\n", my_function(123));

- When you are done with a library, call MOJOELF_dlclose() to free any
  resources. All pointers returned by MOJOELF_dlsym() for this library are
  invalid after this call.

- Other fun stuff: MOJOELF_getentry() gets you the entry point for the
  ELF file (which isn't useful on shared libraries, but is how you eventually
  get to main() in an executable).



Callbacks:

You supply a handful of callbacks when calling MOJOELF_dlopen_*(). Any given
callback is allowed to be NULL, as is the pointer to the MOJOELF_callbacks if
you don't care about these at all. The function pointers get copied, so you
can free the MOJOELF_Callbacks struct as soon as MOJOELF_dlopen_*() returns.

    typedef struct MOJOELF_Callbacks
    {
        MOJOELF_LoaderCallback loader;
        MOJOELF_SymbolCallback resolver;
        MOJOELF_UnloaderCallback unloader;
    } MOJOELF_Callbacks;

Note that MOJOELF_dlopen_*() does not make any attempt to resolve dependencies
on its own, so if you want to implement something like Linux's dynamic loader,
you'll need to parse LD_LIBRARY_PATH, or whatever, on your own using these
callbacks. By default, without callbacks, any dependency (including libc.so)
or unresolved symbol will cause MOJOELF_dlopen_*() to fail.


The "loader" callback doesn't necessarily load anything. All it does it tell
MojoELF that it's claiming a specific dependency. For example, if you want to
load an ELF that depends on libFoo.so.3, but you plan to override this
library without it actually existing, you can write a callback like this:

    void *my_loader(const char *soname, const char *rpath, const char *runpath)
    {
        return (void *) (strcmp(soname, "libFoo.so.3") == 0);
    }

...and MojoELF will not try to load libFoo itself, and assumes you will
provide any needed symbols from it via your resolver callback.

Note that your loader can be way more complex...it could actually _load_
something, for example, but in many cases, this is all that's needed.

The loader callback is provided with any RPATH or RUNPATH entries in the
currently-loading ELF. Please refer to Linux's dlopen() manpage for details
on these strings.

The value returned from the loader callback is opaque data. It will be passed
to your resolver callback, and is expected to be free'd in the unloader
callback, if you like.


The resolver callback looks like this:

      extern int my_function(int argument);

      void *my_resolver(void *handle, const char *sym)
      {
          if (strcmp(sym, "my_function") == 0)
              return my_function;
          // this also works for data, not just functions.
          return NULL;  /* can't help you. */
      }

The callback is called once for each non-NULL value that your loader
previously returned, in the other they were returned, until one succeeds. If
none succeeds, the callback fires one more time with the handle set to NULL.
If this still doesn't return a non-NULL value, MojoELF will fail to load the
ELF file, due to missing dependencies.


The unloader callback is like this:

      void my_unloader(void *handle)
      {
          // if (handle) is something you allocated in your loader callback,
          //  you can free it here.
      }


- If you have problems: ask Ryan.

