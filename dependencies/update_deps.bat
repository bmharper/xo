@echo off
if not '%1'=='' goto haveSRC
	echo You must specify the source, such as c:\dev\head\otaku
	goto :eof
:haveSRC
xcopy /y %1\proj\Panacea\coredefs.h dependencies\Panacea\
xcopy /y %1\proj\Panacea\warnings.h dependencies\Panacea\
xcopy /y %1\proj\Panacea\disable_all_code_analysis_warnings.h dependencies\Panacea\
xcopy /y %1\proj\Panacea\Bits\BitMap.h dependencies\Panacea\Bits\
xcopy /y %1\proj\Panacea\Containers\* dependencies\Panacea\Containers\
xcopy /y %1\proj\Panacea\Core\DataSig.h dependencies\Panacea\Core\
xcopy /y %1\proj\Panacea\fhash\fhashtable.h dependencies\Panacea\fhash\
xcopy /y %1\proj\Panacea\HashCrypt\* dependencies\Panacea\HashCrypt\
xcopy /y %1\proj\Panacea\HashTab\* dependencies\Panacea\HashTab\
xcopy /y %1\proj\Panacea\IO\* dependencies\Panacea\IO\
xcopy /y %1\proj\Panacea\modp\src\* dependencies\Panacea\modp\src\
xcopy /y %1\proj\Panacea\murmur3\MurmurHash3.h dependencies\Panacea\murmur3\
xcopy /y %1\proj\Panacea\Other\* dependencies\Panacea\Other\
xcopy /y %1\proj\Panacea\Platform\* dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Strings\* dependencies\Panacea\Strings\
xcopy /y %1\proj\Panacea\System\Date.cpp dependencies\Panacea\System\
xcopy /y %1\proj\Panacea\System\Date.h dependencies\Panacea\System\
xcopy /y %1\proj\Panacea\Vec\* dependencies\Panacea\Vec\
xcopy /y %1\proj\Panacea\Windows\Win.cpp dependencies\Panacea\Windows\
xcopy /y %1\proj\Panacea\Windows\Win.h dependencies\Panacea\Windows\

xcopy /y %1\proj\TinyTest\* dependencies\TinyTest\

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
