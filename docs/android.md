GCC alignment issue
-------------------

I don't know what is causing this, but certain classes (or structs) cause SIGBUS
code 128 when they are copied with regular C++ assignment. This signal is
caused by data misalignment. I can only blame the compiler for generating
invalid code. This is happening on Samsung S3.
The compiler used here is GCC 4.6, with the NDK r9.
The workaround is to create an explicit assignment operator.
Wherever you see that, it is labelled with $NU_GCC_ALIGN_BUG