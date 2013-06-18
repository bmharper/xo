@echo off
if not '%1'=='' goto haveSRC
	echo You must specify the source, such as c:\dev\head\otaku
	goto :eof
:haveSRC
xcopy /y %1\proj\Panacea\coredefs.h dependencies\Panacea\
xcopy /y %1\proj\Panacea\Bits\BitMap.h dependencies\Panacea\Bits\
xcopy /y %1\proj\Panacea\Other\aligned_malloc.h dependencies\Panacea\Other\
xcopy /y %1\proj\Panacea\Other\StackAllocators.h dependencies\Panacea\Other\
xcopy /y %1\proj\Panacea\Containers\pvect.h dependencies\Panacea\Containers\
xcopy /y %1\proj\Panacea\Containers\podvec.h dependencies\Panacea\Containers\
xcopy /y %1\proj\Panacea\Containers\queue.cpp dependencies\Panacea\Containers\
xcopy /y %1\proj\Panacea\Containers\queue.h dependencies\Panacea\Containers\
xcopy /y %1\proj\Panacea\Containers\cont_utils.h dependencies\Panacea\Containers\
xcopy /y %1\proj\Panacea\fhash\fhashtable.h dependencies\Panacea\fhash\
xcopy /y %1\proj\Panacea\Platform\compiler.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\err.cpp dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\err.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\stdint.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\syncprims.cpp dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\syncprims.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\timeprims.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Strings\fmt.cpp dependencies\Panacea\Strings\
xcopy /y %1\proj\Panacea\Strings\fmt.h dependencies\Panacea\Strings\
xcopy /y %1\proj\Panacea\Vec\Vec2.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\Vec3.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\Vec4.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\Mat2.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\Mat3.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\Mat4.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\VecPrim.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\VecDef.h dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Vec\VecUndef.h dependencies\Panacea\Vec\

rem xcopy /y %1\third_party\biggle\tmp\biggle.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\biggle.h dependencies\biggle\
rem 
rem xcopy /y %1\third_party\biggle\tmp\glUniform1f.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glUniform4f.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glGetAttribLocation.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glGetUniformLocation.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glUseProgram.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glUniformMatrix4fv.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glUniform2f.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glEnableVertexAttribArray.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glVertexAttribPointer.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glDeleteShader.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glGetProgramInfoLog.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glGetProgramiv.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glLinkProgram.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glAttachShader.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glCreateProgram.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glGetShaderInfoLog.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glGetShaderiv.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glCompileShader.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glShaderSource.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\glCreateShader.c dependencies\biggle\
rem xcopy /y %1\third_party\biggle\tmp\wglGetExtensionsStringARB.c dependencies\biggle\
