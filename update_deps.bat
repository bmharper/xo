@echo off
xcopy /y ..\Panacea\coredefs.h dependencies\Panacea\
xcopy /y ..\Panacea\Bits\BitMap.h dependencies\Panacea\Bits\
xcopy /y ..\Panacea\Other\lmstdint.h dependencies\Panacea\Other\
xcopy /y ..\Panacea\Other\aligned_malloc.h dependencies\Panacea\Other\
xcopy /y ..\Panacea\Containers\pvect.h dependencies\Panacea\Containers\
xcopy /y ..\Panacea\Containers\podvec.h dependencies\Panacea\Containers\
xcopy /y ..\Panacea\Containers\queue.cpp dependencies\Panacea\Containers\
xcopy /y ..\Panacea\Containers\queue.h dependencies\Panacea\Containers\
xcopy /y ..\Panacea\Containers\cont_utils.h dependencies\Panacea\Containers\
xcopy /y ..\Panacea\fhash\fhashtable.h dependencies\Panacea\fhash\
xcopy /y ..\Panacea\Platform\syncprims.cpp dependencies\Panacea\Platform\
xcopy /y ..\Panacea\Platform\syncprims.h dependencies\Panacea\Platform\
xcopy /y ..\Panacea\Platform\err.cpp dependencies\Panacea\Platform\
xcopy /y ..\Panacea\Platform\err.h dependencies\Panacea\Platform\
xcopy /y ..\Panacea\Vec\Vec2.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\Vec3.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\Vec4.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\Mat2.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\Mat3.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\Mat4.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\VecPrim.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\VecDef.h dependencies\Panacea\Vec\
xcopy /y ..\Panacea\Vec\VecUndef.h dependencies\Panacea\Vec\

rem xcopy /y ..\..\third_party\biggle\tmp\biggle.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\biggle.h dependencies\biggle\
rem 
rem xcopy /y ..\..\third_party\biggle\tmp\glUniform1f.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glUniform4f.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glGetAttribLocation.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glGetUniformLocation.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glUseProgram.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glUniformMatrix4fv.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glUniform2f.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glEnableVertexAttribArray.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glVertexAttribPointer.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glDeleteShader.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glGetProgramInfoLog.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glGetProgramiv.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glLinkProgram.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glAttachShader.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glCreateProgram.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glGetShaderInfoLog.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glGetShaderiv.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glCompileShader.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glShaderSource.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\glCreateShader.c dependencies\biggle\
rem xcopy /y ..\..\third_party\biggle\tmp\wglGetExtensionsStringARB.c dependencies\biggle\
