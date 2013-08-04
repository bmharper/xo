#include "pch.h"
#include "lmTypes.h"
#include "TestUtils.h"
#include "Vec/IRect.h"

static void Blah()
{
	AbCore::IRect<INT32> a;
	a.Reset();
	AbCore::IRect<UINT32> b;
	b.Reset();
	AbCore::IRect<INT64> c;
	c.Reset();
	AbCore::IRect<UINT64> d;
	d.Reset();
}

namespace AbCore
{
namespace Test
{

#ifdef _WIN32

struct SpecialAlloc
{
	void* Block;
	void* GuardPage;
};

const size_t SysPageSize = 4096;

static int AccessViolationCount = 0;
static int AccessViolation_Read_Count = 0;
static int AccessViolation_Write_Count = 0;
static SpecialAlloc* CurrentGuardBlock = NULL;

PAPI void Exception_InstallHandler();


/** Allocates a block of memory, and places a guard page immediately after it.

Layout:

First Page                    Page Boundary
		^                              ^                 
		| [SpecialAlloc] [User Memory] | [Guard Page]

**/
PAPI void* SpecialAlloc_GuardAtEnd( size_t bytes )
{
	if ( CurrentGuardBlock )
	{
		// One could design this to be able to have multiple guard blocks in flight at once. I would do it 
		// by making SpecialAlloc a linked list. I have not yet needed that.
		assert(false);
		return NULL;
	}

	Exception_InstallHandler();

	size_t bytesTot = bytes + sizeof(SpecialAlloc);
	
	// Number of pages that we need for user space.
	// This is [sizeof(SpecialAlloc) + bytes]
	size_t pages = (bytesTot + SysPageSize - 1) / SysPageSize;

	// Add 1 for the guard page
	pages++;

	char* base = (char*) VirtualAlloc( NULL, pages * SysPageSize, MEM_RESERVE, PAGE_NOACCESS );
	ASSERT( base != 0 );

	// Legal Top is the start of the guard page
	char* legalTop = base + (pages - 1) * SysPageSize;

	// Commit the base pages
	void* pp1 = VirtualAlloc( base, (pages - 1) * SysPageSize, MEM_COMMIT, PAGE_READWRITE );

	// ... but delay commitment of the guard page for the exception handler.

	// Write our custom block structure immediately behind the address that we give to the user
	char* user = legalTop - bytes;
	SpecialAlloc* sp = (SpecialAlloc*) (user - sizeof(SpecialAlloc));
	sp->Block = base;
	sp->GuardPage = legalTop;
	CurrentGuardBlock = sp;

	return user;
}

PAPI void SpecialFree( void* block )
{
	CurrentGuardBlock = NULL;
	SpecialAlloc* sp = (SpecialAlloc*) ((char*) block - sizeof(SpecialAlloc));
	BOOL ok = VirtualFree( sp->Block, 0, MEM_RELEASE );
	ASSERT( ok );
}

LONG WINAPI HandleException( EXCEPTION_POINTERS* ex )
{
	if ( ex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION )
	{
		AccessViolationCount++;
		if ( ex->ExceptionRecord->ExceptionInformation[0] == 0 )
			AccessViolation_Read_Count++;
		else
			AccessViolation_Write_Count++;
		
		if ( CurrentGuardBlock )
		{
			void* pp1 = VirtualAlloc( CurrentGuardBlock->GuardPage, SysPageSize, MEM_COMMIT, PAGE_READWRITE );
			//printf( "Committing guard block (%d)\n", (size_t) pp1 );
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}


PAPI void Exception_InstallHandler()
{
	AccessViolationCount = 0;
	AccessViolation_Read_Count = 0;
	AccessViolation_Write_Count = 0;
	LPTOP_LEVEL_EXCEPTION_FILTER oldFilter = SetUnhandledExceptionFilter( &HandleException );
}

PAPI int Exception_AccessViolationCount( int* read_violations, int* write_violations )
{
	if ( read_violations ) *read_violations = AccessViolation_Read_Count;
	if ( write_violations ) *write_violations = AccessViolation_Write_Count;
	return AccessViolationCount;
}

#endif

}
}
