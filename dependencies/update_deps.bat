@echo off
if not '%1'=='' goto haveSRC
	echo You must specify the source, such as c:\dev\head\otaku
	goto :eof
:haveSRC
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
xcopy /y %1\proj\Panacea\Platform\coredefs.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\cpu.cpp dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\cpu.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\err.cpp dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\err.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\stdint.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\syncprims.cpp dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\syncprims.h dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\thread.cpp dependencies\Panacea\Platform\
xcopy /y %1\proj\Panacea\Platform\thread.h dependencies\Panacea\Platform\
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


xcopy /y %1\proj\TinyTest\* dependencies\TinyTest\
