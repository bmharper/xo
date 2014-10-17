# Debugging

On Windows this is dead simple, just use Visual Studio.

For other platforms, you can use "printf" style debugging.
At the bottom of xoDefs.h are a bunch of XOTRACE_XXX macros, which are usually
just no-ops, but by turning them into trace commands, you can enable big
chunks of traces inside the xo core.

On Android I have successfully used gdb from Windows like so:
C:\dev\android-ndk-r10b\ndk-gdb-py --adb="C:\Program Files (x86)\Android\android-studio\sdk\platform-tools\adb.exe" --start

You need to build with:
	ndk-build NDK_DEBUG=1

You will probably need to close Eclipse or IntelliJ (aka Android Studio) so that
gdb can get control of the debugging interface.

Sample path for setting a breakpoint:
b jni/../../../xo/xoLayout.cpp:41
b jni/../../../xo/xoDocGroup.cpp:160
b jni/../../../xo/xoDocGroup.cpp:51
b jni/../../../xo/Text/xoGlyphcache.cpp:91

After setting a breakpoint at program startup, I have to single-step through 3 times (press "s"). Pressing "c" immediately didn't work.
