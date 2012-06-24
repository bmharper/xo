#include <stdio.h>
#include <algorithm>
#include <limits>
#include <assert.h>
#include <pthread.h>

typedef const wchar_t* LPCTSTR;
typedef const char* LPCSTR;
#define PAPI
#ifdef _DEBUG
#define ASSERT(condition) (void)0
#else
#define ASSERT(condition) assert(condition)
#endif
#include "../dependencies/Panacea/coredefs.h"
#include "../dependencies/Panacea/Platform/err.h"
#include "../dependencies/Panacea/Other/lmstdint.h"
#include "../dependencies/Panacea/Containers/pvect.h"
#include "../dependencies/Panacea/Containers/podvec.h"
#include "../dependencies/Panacea/Containers/queue.h"
#include "../dependencies/Panacea/fhash/fhashtable.h"
#include "../dependencies/Panacea/Platform/syncprims.h"

static void HelloAndroid123()
{
}