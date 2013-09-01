# Debugging

On Windows this is dead simple, just use Visual Studio.

For other platforms, you can use "printf" style debugging.
At the bottom of nuDefs.h are a bunch of NUTRACE_XXX macros, which are usually
just no-ops, but by turning them into trace commands, you can enable big
chunks of traces inside the nudom core.

On Android I have successfully used gdb from Windows like so:
c:\dev\android-ndk-r9\ndk-gdb-py --adb="c:\Program Files (x86)\Android\android-sdk\platform-tools\adb.exe" --start

You will probably need to close Eclipse or IntelliJ (aka Android Studio) so that
gdb can get control of the debugging interface.

jni/../../../nudom/nuLayout.cpp:41