@echo off
@rem "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

cl /nologo /Zi /I.. /I../.. HelloWorld.cpp user32.lib Advapi32.lib
