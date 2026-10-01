# MojoELF

MojoELF is an ELF binary loader that runs in your application instead of
as part of the C runtime. The most useful feature of this is that, unlike
the standard `dlopen()`, it can load an ELF file from a place other than
the filesystem. Notably, it can load one from a buffer in memory.


## Documentation

You can read it [on the wiki](https://wiki.icculus.org/MojoELF), or read
mojoelf.h (which shares the exact same content).


## If you have problems or questions

Please [file a bug](https://github.com/icculus/mojoelf/issues). Thanks!

