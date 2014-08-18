# Debugging

On Windows this is dead simple, just use Visual Studio.

For other platforms, you can use "printf" style debugging.
At the bottom of xoDefs.h are a bunch of XOTRACE_XXX macros, which are usually
just no-ops, but by turning them into trace commands, you can enable big
chunks of traces inside the xo core.

On Android I have successfully used gdb from Windows like so:
C:\dev\sdk\android-ndk-r9b\ndk-gdb-py --adb="C:\Program Files (x86)\Android\android-studio\sdk\platform-tools\adb.exe" --start

You will probably need to close Eclipse or IntelliJ (aka Android Studio) so that
gdb can get control of the debugging interface.

Sample path for setting a breakpoint:
jni/../../../xo/xoLayout.cpp:41