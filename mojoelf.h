/*
 * MojoELF; load ELF binaries from a memory buffer.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

/* WIKI CATEGORY: MojoELF */

/**
 * # CategoryMojoELF
 *
 * The latest version of MojoELF can be found at:
 *
 * https://github.com/icculus/mojoelf
 *
 * MojoELF is an ELF binary loader that runs in your application, instead of
 * as part of the C runtime. Its most useful feature is that, unlike the
 * standard `dlopen()`, it can load an ELF file from a place other than the
 * filesystem. Notably, it can load one from a buffer in memory.
 *
 * It can also be useful for overriding specific parts of the dependency
 * management process, such as loading a replacement library or quietly
 * replacing a function, etc.
 *
 * To use MojoELF:
 *
 * - Add mojoelf.c to your build. MojoELF is intended to be compiled directly
 *   into your project as a single C file, not built as an external shared
 *   library (but you could probably do that if you like).
 * - Compile with these #defines set as you please, or change the top of
 *   mojoelf.c:
 *
 * ```c
 *   #define MOJOELF_SUPPORT_DLERROR 0  // remove MOJOELF_dlerror() + lots of strings.
 *   #define MOJOELF_SUPPORT_DLOPEN_FILE 0 // remove MOJOELF_dlopen_file()
 *   #define MOJOELF_REDUCE_LIBC_DEPENDENCIES 0  // use less libc calls. Scary!
 *   #define NDEBUG 1  // Turns off assert, which removes libc dependencies.
 * ```
 *
 * - Your calling code should `#include mojoelf.h` - Put your ELF library in
 *   memory, and call MOJOELF_dlopen_mem() with the address of the memory
 *   buffer, the size of the buffer, and (optionally), callbacks that handle
 *   symbol resolution (they can be NULL). Refer to the documentation on
 *   MOJOELF_LoaderCallback, MOJOELF_ResolverCallback, and
 *   MOJOELF_UnloaderCallback for details.
 * - If MOJOELF_dlopen_mem() returns non-NULL, the library is ready to use. If
 *   it returns NULL, there was a problem (MOJOELF_dlerror() can give you a
 *   human-readable error message). On success, you can free your buffer; we
 *   don't need it after MOJOELF_dlopen_mem() returns.
 * - MOJOELF_dlopen_file() does the same thing, but takes a filename instead
 *   of a memory buffer. Internally, it just loads the file into a malloc()'d
 *   buffer and calls MOJOELF_dlopen_mem().
 * - To request entry points into the library, use MOJOELF_dlsym():
 *
 * ```c
 * typedef int (*my_fn_type)(int argument);
 * my_fn_type my_function = (my_fn_type) MOJOELF_dlsym(lib, "AwesomeFunc");
 * if (my_function == NULL) {
 *     printf("couldn't find AwesomeFunc!\n");
 * } else {
 *     printf("AwesomeFunc() returns %d\n", my_function(123));
 * }
 * ```
 *
 * - When you are done with a library, call `MOJOELF_dlclose()` to free any
 *   resources. All pointers returned by `MOJOELF_dlsym()` for this library
 *   are invalid after this call. - Other fun stuff: `MOJOELF_getentry()` gets
 *   you the entry point for the ELF file (which isn't useful on shared
 *   libraries, but is how you eventually get to main() in an executable).
 *
 * Thread safety:
 *
 * MojoELF provides no locking mechanisms, and has a piece of global state
 * that almost any function can set (the MOJOELF_dlerror() state). As such,
 * the app is responsible for providing a serialization method if they plan to
 * use MojoELF from multiple threads at once.
 *
 * Other stuff:
 *
 * Please see the file LICENSE.txt in the source's root directory for
 * licensing and redistribution rights.
 */

#ifndef INCL_MOJOELF_H
#define INCL_MOJOELF_H

#ifdef __cplusplus
extern "C" {
#endif

// initial API setup stuff...

#ifndef MOJOELF_DECL

/**
 * A macro to tag a symbol as a public API.
 *
 * MojoELF uses this macro for all its public functions. On some targets, it
 * is used to signal to the compiler that this function needs to be exported
 * from a shared library, but it might have other side effects.
 *
 * Generally one compiles MojoELF into a project directly and doesn't
 * dynamically link it, so MOJOELF_DECL is intentionally blank.
 *
 * It's here in case you _must_ override it for whatever reason, and because
 * wikiheaders.pl uses this to identify function signatures in this header
 * when generating documentation. You can probably ignore it.
 *
 * \since This macro is available since MojoELF 1.0.0.
 */
#define MOJOELF_DECL
#endif

#ifndef MOJOELF_CALL

/**
 * The calling conventions for MojoELF entry points.
 *
 * This is currently defined to nothing on all platforms and compilers. As
 * such, at this time it means all APIs and callbacks use the compiler's
 * default calling conventions ("cdecl" or whatever).
 *
 * This is for future expansion, and compatibility with wikiheaders.pl.
 *
 * \since This macro is available since MojoELF 1.0.0.
 */
#define MOJOELF_CALL
#endif


// Versioning.

/**
 * The current major version of MojoELF headers.
 *
 * If this were MojoELF version 3.2.1, this value would be 3.
 *
 * \since This macro is available since MojoELF 1.0.0.
 */
#define MOJOELF_MAJOR_VERSION   0

/**
 * The current minor version of the MojoELF headers.
 *
 * If this were MojoELF version 3.2.1, this value would be 2.
 *
 * \since This macro is available since MojoELF 1.0.0.
 */
#define MOJOELF_MINOR_VERSION   0

/**
 * The current micro (or patchlevel) version of the MojoELF headers.
 *
 * If this were MojoELF version 3.2.1, this value would be 1.
 *
 * \since This macro is available since MojoELF 1.0.0.
 */
#define MOJOELF_MICRO_VERSION   9


// callbacks used by MOJOELF_dlopen_* ...

/**
 * A callback used when MojoELF needs to load a new dependency.
 *
 * The "loader" callback doesn't necessarily load anything. All it does it
 * tell MojoELF that it's claiming a specific dependency. For example, if you
 * want to load an ELF that depends on libFoo.so.3, but you plan to override
 * this library without it actually existing, you can write a callback like
 * this...
 *
 * ```c
 * void *my_loader(const char *soname, const char *rpath, const char *runpath)
 * {
 *     return (void *) (strcmp(soname, "libFoo.so.3") == 0);
 * }
 * ```
 *
 * ...and MojoELF will not try to load libFoo itself, and assumes you will
 * provide any needed symbols from it via your MOJOELF_ResolverCallback.
 *
 * Note that your loader can be significantly more complex...it could actually
 * _load_ something, for example, but for some projects, this is all that's
 * needed. What it chooses to load (even if it violates the norms of an ELF
 * loader) is entirely up to its discretion.
 *
 * The loader callback is provided with any `RPATH` or `RUNPATH` entries in
 * the currently-loading ELF, in case finding the correct shared library might
 * need these. Please refer to Linux's `dlopen()` manpage for details on these
 * strings.
 *
 * The value returned from the loader callback is opaque data. It will be
 * passed to your MOJOELF_ResolverCallback, and is expected to be free'd in
 * the MOJOELF_UnloaderCallback, if you like.
 *
 * If this callback returns NULL, the library that MojoELF was in the process
 * of loading will fail to load in response.
 *
 * \param soname the SONAME of the ELF library to load.
 * \param rpath the RPATH entry of the ELF library that needs this dependency.
 *              Will be NULL if the library had no such entry.
 * \param runpath the RUNPATH entry of the ELF library that needs this
 *                dependency. Will be NULL if the library had no such entry.
 * \returns non-NULL on success, NULL on failure. The returned non-NULL
 *          pointer must live until the paired MOJOELF_UnloaderCallback.
 *
 * \threadsafety This will be called from the same thread that called into
 *               `MOJOELF_dlopen_*`. Locking is not provided by MojoELF.
 *
 * \since This function is available since MojoELF 1.0.0.
 *
 * \sa MOJOELF_ResolverCallback
 * \sa MOJOELF_UnloaderCallback
 */
typedef void *(*MOJOELF_LoaderCallback)(const char *soname, const char *rpath, const char *runpath);

/**
 * A callback used when MojoELF needs to resolve a symbol.
 *
 * This passes the pointer returned by MOJOELF_LoaderCallback as `handle` and
 * will call this callback once for each loaded library until it finds one
 * that resolves the symbol or runs out of libraries to try.
 *
 * The callback is called once for each non-NULL value that your loader
 * previously returned, in the other they were returned, until one succeeds.
 * If none succeeds, the callback fires one more time with the handle set to
 * NULL. If this still doesn't return a non-NULL value, MojoELF will give up
 * on the ELF file it was in the process of loading, due to missing
 * dependencies.
 *
 * A simple resolver callback might look like this:
 *
 * ```c
 * extern int my_function(int argument);
 *
 * void *my_resolver(void *handle, const char *sym)
 * {
 *   if (strcmp(sym, "my_function") == 0)
 *       return my_function;
 *   // this also works for data, not just functions.
 *   return NULL;  // can't help you.
 * }
 * ```
 *
 * As you can see, it isn't _required_ to build a whole resolving
 * infrastructure if one just wants to provide a handful of specific entry
 * points.
 *
 * Note: this callback returns NULL to specify "symbol not found" but strictly
 * speaking, a symbol _could_ exist at address 0x00000000. In practice, this
 * would be ridiculous, so MojoELF makes no effort to allow this scenario.
 *
 * \param handle a pointer returned from a previous MOJOELF_LoaderCallback, or
 *               NULL when all other options have been exhausted.
 * \param sym the symbol to find an address for.
 * \returns address of symbol if found, NULL if symbol isn't available.
 *
 * \threadsafety This will be called from the same thread that called into
 *               `MOJOELF_dlopen_*`. Locking is not provided by MojoELF.
 *
 * \since This function is available since MojoELF 1.0.0.
 *
 * \sa MOJOELF_LoaderCallback
 * \sa MOJOELF_UnloaderCallback
 */
typedef void *(*MOJOELF_ResolverCallback)(void *handle, const char *sym);

/**
 * A callback used when MojoELF is done with a dependency.
 *
 * This is called during MOJOELF_dlclose(), once for each dependency that a
 * MOJOELF_LoaderCallback successfully loaded. It will also be called during
 * the `MOJOELF_dlopen_*` functions if they fail, to clean up the
 * half-initialized state.
 *
 * The callback should dispose of any resources allocated during the paired
 * MOJOELF_LoaderCallback. After this call returns, MojoELF will not use this
 * copy of `handle` again.
 *
 * \param handle a pointer returned from a previous MOJOELF_LoaderCallback, to
 *               be destroyed.
 *
 * \threadsafety This will be called from the same thread that called into
 *               `MOJOELF_dlopen_*` or MOJOELF_dlclose(). Locking is not
 *               provided by MojoELF.
 *
 * \since This function is available since MojoELF 1.0.0.
 *
 * \sa MOJOELF_ResolverCallback
 * \sa MOJOELF_UnloaderCallback
 */
typedef void (*MOJOELF_UnloaderCallback)(void *handle);

/**
 * A struct for passing a set of callbacks into MojoELF.
 *
 * This exists to gather all callbacks up and pass a single pointer to the
 * dlopen functions.
 *
 * \since This function is available since MojoELF 1.0.0.
 *
 * \sa MOJOELF_LoaderCallback
 * \sa MOJOELF_ResolverCallback
 * \sa MOJOELF_UnloaderCallback
 * \sa MOJOELF_dlopen_mem
 * \sa MOJOELF_dlopen_file
 */
typedef struct MOJOELF_Callbacks
{
    MOJOELF_LoaderCallback loader;  /**< loads dependencies during a dlopen */
    MOJOELF_ResolverCallback resolver;  /**< resolves dependencies during a dlopen */
    MOJOELF_UnloaderCallback unloader;  /**< unloads dependencies during dlclose */
} MOJOELF_Callbacks;

/**
 * Dynamically load an ELF binary from memory.
 *
 * This will load binary's code and data into appropriate places in the
 * address space, load any dependencies it specifies as well, fixup any
 * symbols necessary, and prepare a table of exported symbols for later
 * lookup.
 *
 * If there is initialization code in the ELF binary (a DT_INIT section, etc),
 * and the loading was otherwise successful, that code will run before this
 * function returns.
 *
 * Callbacks are not required (any specific function may safely be NULL, and
 * the struct itself may be NULL as well), but loading an ELF of almost any
 * complexity will fail without a means to load dependencies and resolve
 * symbols in them.
 *
 * On success, this returns an opaque handle to the newly-opened binary. On
 * failure, this returns NULL, and MOJOELF_dlerror() can be used to get a
 * human-readable reason why.
 *
 * A valid handle can than be used with MOJOELF_dlsym() to find the location
 * of the ELF's symbols in memory, such as to find a function pointer to call
 * through.
 *
 * When done with the returned handle, pass it to MOJOELF_dlclose() to dispose
 * of it. The pointer returned here is not compatible with the C runtime's
 * dlclose(), dlsym(), etc; only use it with MojoELF functions!
 *
 * This function copies the data in `buf` as appropriate; the original buffer
 * can be disposed of once this function returns.
 *
 * \param buf the bytes of a valid ELF binary to be loaded.
 * \param buflen the number of bytes pointed to by `buf`.
 * \param cb callbacks that manage how the ELF loader operates. May be NULL,
 *           and any function pointer in the struct may also individually be
 *           NULL.
 * \returns a non-NULL pointer on success, NULL on failure. On failure, a
 *          human-readable error message may be obtained by calling
 *          MOJOELF_dlerror().
 *
 * \threadsafety this function is not thread-safe.
 *
 * \since This function is available since MojoELF 1.0.0.
 *
 * \sa MOJOELF_dlopen_file
 * \sa MOJOELF_dlsym
 * \sa MOJOELF_dlclose
 */
extern MOJOELF_DECL void * MOJOELF_CALL MOJOELF_dlopen_mem(const void *buf, const long buflen, const MOJOELF_Callbacks *cb);

/**
 * Dynamically load an ELF binary from a path on the filesystem.
 *
 * This is a convenience function that allocates a temporary memory buffer,
 * loads `fname` into it, calls MOJOELF_dlopen_mem(), and then frees the
 * temporary buffer.
 *
 * All the same information in that function's documentation applies here.
 *
 * \param fname the path to the file to load on the filesystem.
 * \param cb callbacks that manage how the ELF loader operates. May be NULL,
 *           and any function pointer in the struct may also individually be
 *           NULL.
 * \returns a non-NULL pointer on success, NULL on failure. On failure, a
 *          human-readable error message may be obtained by calling
 *          MOJOELF_dlerror().
 *
 * \threadsafety this function is not thread-safe.
 *
 * \since This function is available since MojoELF 1.0.0.
 *
 * \sa MOJOELF_dlopen_mem
 * \sa MOJOELF_dlsym
 * \sa MOJOELF_dlclose
 */
extern MOJOELF_DECL void * MOJOELF_CALL MOJOELF_dlopen_file(const char *fname, const MOJOELF_Callbacks *cb);

/**
 * Lookup a symbol in an ELF binary.
 *
 * The `lib` param must be a pointer returned from MOJOELF_dlopen_mem() or
 * MOJOELF_dlopen_file(). You can not use a handle returned from the C
 * runtime's dlopen() function here!
 *
 * A NULL return value means the symbol was not found. Strictly speaking,
 * though, a symbol _could_ have a valid address of NULL--zero--but this is
 * not allowed in MojoELF, and would be extremely foolish anyhow.
 *
 * \param lib a handle from a MojoELF-opened ELF binary.
 * \param sym the symbol to look up.
 * \returns non-NULL address of the symbol if found, NULL if not found. On
 *          failure, a human-readable error message may be obtained by calling
 *          MOJOELF_dlerror().
 *
 * \threadsafety this function is not thread-safe.
 *
 * \since This function is available since MojoELF 1.0.0.
 */
extern MOJOELF_DECL void * MOJOELF_CALL MOJOELF_dlsym(void *lib, const char *sym);

/**
 * Dispose of a previously-loaded ELF binary.
 *
 * This will free any resources involved with this ELF binary, including
 * calling into a MOJOELF_UnloaderCallback to clean out any dependencies that
 * were loaded with the binary.
 *
 * The `lib` param must be a pointer returned from MOJOELF_dlopen_mem() or
 * MOJOELF_dlopen_file(). You can not use a handle returned from the C
 * runtime's dlopen() function here!
 *
 * MojoELF does not currently reference-count loaded ELF binaries; a single
 * call to this function will destroy the binary's handle (and multiple loads
 * of the same library will all be separate instances of it, that are closed
 * separately). Once this function is called, the app should consider `lib`
 * invalid and not use it again.
 *
 * Calling this function with a NULL `lib` is a valid no-op.
 *
 * \param lib the ELF binary of which to dispose.
 *
 * \threadsafety this function is not thread-safe.
 *
 * \since This function is available since MojoELF 1.0.0.
 *
 * \sa MOJOELF_dlopen_mem
 * \sa MOJOELF_dlopen_file
 */
extern MOJOELF_DECL void MOJOELF_CALL MOJOELF_dlclose(void *lib);

/**
 * Obtain a human-readable error message for MojoELF problems.
 *
 * This function returns a description of the last error that has occured in a
 * call to a MojoELF function.
 *
 * The returned string is owned by MojoELF and should not be free'd by the
 * caller. This string lives at least until the next call into any of
 * MojoELF's functions.
 *
 * Once a string is returned by this function, future calls to this function
 * will return NULL until another error has occured. If no error has _ever_
 * occured, NULL is returned. The correct usage pattern is to call this
 * immediately after receiving a failing result from a function call. It is
 * incorrect to call this function as a way to decide if a function call has
 * failed.
 *
 * Note that MojoELF can be built so that this function always returns NULL,
 * which removes a lot of string data from the build, for minimizing the
 * program's binary size, etc. Doing this requires explicit intervention from
 * the app at build time, though, and is not the default.
 *
 * \returns human-readable string of last error, or NULL if no error has
 *          occured since the last call to this function.
 *
 * \threadsafety this function is not thread-safe.
 *
 * \since This function is available since MojoELF 1.0.0.
 */
extern MOJOELF_DECL const char * MOJOELF_CALL MOJOELF_dlerror(void);

/**
 * Obtain an ELF binary's official entry point.
 *
 * You almost certainly do not want or need this function.
 *
 * This returns the address where a process should start executing (which
 * would, in most cases, perform some initialization code that eventually
 * calls the `main` function).
 *
 * Not all ELF binaries have an entry point, in which case this will return
 * NULL, but this scenario is not an error in itself. Shared libraries, the
 * most common thing one would load, often do not have an entry point (but
 * they may! One can run glibc's libc.so on a Linux box as if it were a
 * program and it will print information to stdout.) Binaries meant to be run
 * as standalone programs always have an entry point, out of necessity.
 *
 * ELF binaries might have initialization code that runs at load time (DT_INIT
 * sections, etc), which is unrelated to the entry point, and is handled
 * during MOJOELF_dlopen_mem() or MOJOELF_dlopen_file().
 *
 * If you are looking to load a shared library and call into a function in it,
 * this is not the function you should call. Instead, obtain the function's
 * address through a call to MOJOELF_dlsym(). This function is useful if you
 * want to load and run an ELF program, which has other complications beyond
 * just calling into a function pointer.
 *
 * Passing in a NULL `lib` will return NULL.
 *
 * \param lib a handle from a MojoELF-opened ELF binary. May be NULL.
 * \returns the address of the ELF binary's entry point.
 *
 * \threadsafety this function is not thread-safe.
 *
 * \since This function is available since MojoELF 1.0.0.
 */
extern MOJOELF_DECL const void * MOJOELF_CALL MOJOELF_getentry(void *lib);

/**
 * Determine the full address space of a loaded ELF binary.
 *
 * An ELF binary is loaded to specific addresses with mmap(). This function
 * returns the starting address of that mapped range, and the number of bytes
 * mapped. This tells you the block of memory that the the ELF is occupying.
 *
 * What is at a specific address in that memory block, whether it is code or
 * data, and what memory protections any given page has, are not guaranteed.
 * It is not a copy of the ELF binary, but different sections of it mapped as
 * specified in the original binary.
 *
 * Most applications don't need to use this function.
 *
 * \param lib a handle from a MojoELF-opened ELF binary.
 * \param addr on return, contains the mmap'd base address for `lib`.
 * \param len on return, set to the number of bytes mmap'd for `lib`.
 *
 * \threadsafety this function is not thread-safe.
 *
 * \since This function is available since MojoELF 1.0.0.
 */
extern MOJOELF_DECL void MOJOELF_CALL MOJOELF_getmmaprange(void *lib, void **addr, unsigned long *len);

#ifdef __cplusplus
}
#endif

#endif

/* end of mojoelf.h ... */

