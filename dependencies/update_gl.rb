funcs = %w(
glActiveTexture
glAttachShader
glBindFragDataLocation
glBlendFuncSeparate
glCompileShader
glCreateProgram
glCreateShader
glDeleteShader
glEnableVertexAttribArray
glGenerateMipmap
glGetAttribLocation
glGetFragDataLocation
glGetProgramInfoLog
glGetProgramiv
glGetShaderInfoLog
glGetShaderiv
glGetUniformLocation
glLinkProgram
glShaderSource
glUniform1f
glUniform1i
glUniform2f
glUniform4f
glUniformMatrix4fv
glUseProgram
glVertexAttribPointer
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