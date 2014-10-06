@rem "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
cl /nologo /MP /Os /DXO_AMALGAMATION /I../../xo HelloAmalgamation.cpp ../../templates/xoWinMain.cpp ../../amalgamation/xo-amalgamation.cpp ../../amalgamation/xo-amalgamation-freetype.c kernel32.lib gdi32.lib user32.lib opengl32.lib glu32.lib shell32.lib D3D11.lib d3dcompiler.lib
