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
    MOJOELF_ALLOW_SYSTEM_RESOLVE 0  // don't use dlopen()/dlsym() internally.
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

Callbacks:

The resolver callback looks like this:

      extern int my_function(int argument);

      void *my_resolver(const char *sym)
      {
          if (strcmp(sym, "my_function") == 0)
              return my_function;
          // this works for data, not just functions.
          return NULL;  /* can't help you. */
      }

The callback gets first shot at supplying addresses. Failing that, we can
optionally ask the system for symbols.


The "loader" callback doesn't necessarily load anything. All it does it tell
MojoELF that it's claiming a specific dependency. For example, if you want to
load an ELF that depends on libFoo.so.3, but you plan to override this
library without it actually existing, you can write a callback like this:

    int my_loader(const char *soname)
    {
        return (strcmp(soname, "libFoo.so.3") == 0);
    }

...and MojoELF will not try to load libFoo itself, and assumes you will
provide any needed symbols from it via your resolver callback.

Note that your loader can be way more complex...it could actually _load_
something, for example, but in many cases, this is all that's needed.


- If you have problems: ask Ryan.

