@echo off

rem This script was built to use https://bitbucket.org/alfonse/glloadgen/wiki/Home
rem The version originally used was 2.0.2
rem I had to comment out the line 'local lfs = pcall(require, "lfs")' in modules/Styles.lua, because my local lua build
rem has lfs included out of the box.

pushd ..\dependencies
set OUT_DIR=%CD%
pushd C:\dev\tools\glLoadGen_2_0_2
lua LoadGen.lua -style=pointer_c -spec=wgl -profile=core -extfile=%OUT_DIR%\wgl_ext.txt %OUT_DIR%\GL\xo
lua LoadGen.lua -style=pointer_c -spec=glX -profile=core -extfile=%OUT_DIR%\glx_ext.txt %OUT_DIR%\GL\xo
lua LoadGen.lua -style=pointer_c -spec=gl -version=3.3 -profile=core -extfile=%OUT_DIR%\gl_ext.txt %OUT_DIR%\GL\xo
popd
echo #include "pch.h" > x
copy /y x + GL\gl_xo.c GL\gl_xo.cpp
copy /y x + GL\wgl_xo.c GL\wgl_xo.cpp
copy /y x + GL\glx_xo.c GL\glx_xo.cpp
del GL\gl_xo.c
del GL\wgl_xo.c
del GL\glx_xo.c
del x
popd