@echo off

rem This script was built to use https://bitbucket.org/alfonse/glloadgen/wiki/Home
rem HOWEVER, before using that script out of the box, first search in it's source code
rem for "char *extensionName;", an replace that with "const char *extensionName;"
rem The author seems to have abandoned the project, and I just can't be bothered to
rem get a bitbucket account and install Mercurial to send a pull request.

pushd ..\dependencies
set OUT_DIR=%CD%
pushd C:\dev\temp\glLoadGen_2_0_5
lua LoadGen.lua -style=pointer_c -spec=wgl -profile=core -extfile=%OUT_DIR%\wgl_ext.txt %OUT_DIR%\GL\xo
lua LoadGen.lua -style=pointer_c -spec=glX -profile=core -extfile=%OUT_DIR%\glx_ext.txt %OUT_DIR%\GL\xo
lua LoadGen.lua -style=pointer_c -spec=gl -version=3.3 -profile=core -extfile=%OUT_DIR%\gl_ext.txt %OUT_DIR%\GL\xo
popd
echo #include "pch.h" > x
copy /b /y x + GL\gl_xo.c GL\gl_xo.cpp
copy /b /y x + GL\wgl_xo.c GL\wgl_xo_windows.cpp
copy /b /y x + GL\glx_xo.c GL\glx_xo_linux.cpp
rem ren GL\gl_xo.h gl_xo_windows.h
rem ren GL\glx_xo.h glx_xo_windows.h
rem ren GL\wgl_xo.h wgl_xo_windows.h
del GL\gl_xo.c
del GL\wgl_xo.c
del GL\glx_xo.c
del x
popd

echo .
echo If lua is unable to find LoadGen.lua, then you must download it. See the script for details.
