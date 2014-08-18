#pragma once

#ifndef ASSERT
	#define TEMP_ASSERT
	#ifdef _DEBUG
		#define ASSERT(condition) (void)0
	#else
		#define ASSERT(condition) assert(condition)
	#endif
#endif

#include "../dependencies/Panacea/Platform/coredefs.h"
#include "../dependencies/Panacea/Platform/stdint.h"
#include "../dependencies/Panacea/Containers/pvect.h"
#include "../dependencies/Panacea/Containers/podvec.h"
#include "../dependencies/Panacea/Containers/queue.h"
#include "../dependencies/Panacea/Platform/cpu.h"
#include "../dependencies/Panacea/Platform/err.h"
#include "../dependencies/Panacea/Platform/filesystem.h"
#include "../dependencies/Panacea/Platform/process.h"
#include "../dependencies/Panacea/Platform/syncprims.h"
#include "../dependencies/Panacea/Platform/timeprims.h"
#include "../dependencies/Panacea/Platform/thread.h"
#include "../dependencies/Panacea/Other/StackAllocators.h"
#include "../dependencies/Panacea/Bits/BitMap.h"
#include "../dependencies/Panacea/fhash/fhashtable.h"
#include "../dependencies/Panacea/Strings/ConvertUTF.h"
#include "../dependencies/Panacea/Vec/Vec2.h"
#include "../dependencies/Panacea/Vec/Vec3.h"
#include "../dependencies/Panacea/Vec/Vec4.h"
#include "../dependencies/Panacea/Vec/Mat4.h"

#include "../dependencies/hash/xxhash.h"

#ifdef TEMP_ASSERT
	#undef TEMP_ASSERT
	#undef ASSERT
#endif
