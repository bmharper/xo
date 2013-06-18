funcs = %w(
glUniform1f
glUniform4f
glGetAttribLocation
glGetUniformLocation
glUseProgram
glUniformMatrix4fv
glUniform2f
glEnableVertexAttribArray
glVertexAttribPointer
glDeleteShader
glGetProgramInfoLog
glGetProgramiv
glLinkProgram
glAttachShader
glCreateProgram
glGetShaderInfoLog
glGetShaderiv
glCompileShader
glShaderSource
glCreateShader
)

out = ""
out += <<END
#include "pch.h"
END

funcs.each { |f| out += "PFN#{f.upcase}PROC #{f};\n" }

out += "void initGLExt() {\n"
funcs.each { |f| out += "#{f} = (PFN#{f.upcase}PROC) wglGetProcAddress( \"#{f}\" );\n" }
out += "}\n"

File.open( "dependencies/glext.cpp", "w" ) { |f| f.write(out); }