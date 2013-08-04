#include "pch.h"
#include "JitPool.h"
#include <Platform/mmap.h>

#if defined(_MSC_VER)
#pragma warning(disable: 6385)	// getting what appears to be spurious warning from /analyze about 'Functions' having 0 accessible bytes.
#pragma warning(disable: 6387)	// ignoring malloc failure
#pragma warning(disable: 28023) // I don't understand this one
#endif

#if defined(_MSC_VER) && defined(_M_X64)
#define JITPOOL_ENABLE_FUNC_TABLE 1
#endif

namespace AsmJit
{

// Zero = uninitialized
// -1 = OS does not have function (ie running under Wine, 2013-02-09)
// +1 = OS has function
static int OSFuncTableStatus;

#if JITPOOL_ENABLE_FUNC_TABLE
PRUNTIME_FUNCTION JitPool::GetRuntimeFunction( DWORD64 controlPC, PVOID context )
{
	JitPool* jp = (JitPool*) context;

	size_t fp = (size_t) controlPC - (size_t) jp->Pool;

	// Find the function
	for ( int i = 0; i < jp->Count; i++ )
	{
		if ( fp >= jp->Functions[i].Pos && fp < jp->Functions[i].Pos + jp->Functions[i].Size )
		{
			//OutputDebugString( L"JitPool: Exception handler callback found function\n" );
			RUNTIME_FUNCTION* rf = jp->RFTable + i;
			UNWIND_INFO* inf = (UNWIND_INFO*) (jp->Pool + rf->UnwindData);
			UNWIND_CODE* codes = (UNWIND_CODE*) (inf + 1);
			return jp->RFTable + i;
		}
	}

	OutputDebugString( L"JitPool: Exception handler callback failed to find function\n" );

	return NULL;
}

DWORD64 FuncTableId( void* poolBase ) { return ((DWORD64) poolBase) | 3; }

#endif

JitPool::JitPool()
{
	Pool = NULL;
	SigPool = NULL;
	Functions = NULL;
	RFTable = NULL;
	RFInstalled = false;
	SignatureBytes = 0;
	PoolPos = 0;
	PoolSize = 0;
	PoolTotalBytes = 0;
	MaxElements = 0;
	Date = 0;
	Count = 0;
	TrimCount = 0;
}

JitPool::~JitPool()
{
	Reset();
}

void JitPool::Reset()
{
	if ( Pool )
	{
		if ( RFInstalled )
		{
			RFInstalled = false;
#if JITPOOL_ENABLE_FUNC_TABLE
			BOOLEAN ok = RtlDeleteFunctionTable( (RUNTIME_FUNCTION*) FuncTableId(Pool) );
			ASSERT(ok);
#endif
		}
		AbcMMapFree( Pool, PoolTotalBytes );
		Pool = NULL;
	}
	else ASSERT(!RFInstalled);
	PoolPos = 0;
	PoolSize = 0;
	MaxElements = 0;
	Date = 0;
	Count = 0;
	TrimCount = 0;
	delete[] Functions;
	Functions = NULL;
	free(RFTable);
	RFTable = NULL;
	//free(RFUnwind);
	//RFUnwind = NULL;
	free(SigPool);
	SigPool = NULL;
}

void JitPool::Initialize( size_t poolBytes, size_t maxElements, size_t signatureSize, bool enableFuncTable64 )
{
	Reset();
#if JITPOOL_ENABLE_FUNC_TABLE
	if ( !OSHasFuncTable() )
	{
		OutputDebugStringA( "JitPool: Disabling function table, because this OS does not support it\n" );
		enableFuncTable64 = false;
	}
#else
	enableFuncTable64 = false;
#endif
	size_t unwindBytes = enableFuncTable64 ? UnwindSize * maxElements : 0;
	poolBytes = (poolBytes + 15) & ~15;
	PoolTotalBytes = RoundUpToPageSize( poolBytes + unwindBytes );
	Pool = (BYTE*) AbcMMap( PoolTotalBytes, AbcMMapType_Read | AbcMMapType_Write | AbcMMapType_Exec );
	PoolPos = 0;
	PoolSize = poolBytes;
	SignatureBytes = signatureSize;
	MaxElements = maxElements;
	SigPool = (BYTE*) malloc( signatureSize * maxElements );
	Functions = new Func[maxElements];
#if JITPOOL_ENABLE_FUNC_TABLE
	if ( enableFuncTable64 )
	{
		RFTable = (RUNTIME_FUNCTION*) malloc( sizeof(RUNTIME_FUNCTION) * maxElements );
		ASSERT( !RFInstalled );
		BOOLEAN ok = RtlInstallFunctionTableCallback( FuncTableId(Pool), (DWORD64) Pool, (DWORD) PoolSize, &GetRuntimeFunction, this, NULL );
		ASSERT( ok );
		RFInstalled = true;
	}
#endif
}

#if JITPOOL_ENABLE_FUNC_TABLE
bool JitPool::OSHasFuncTable()
{
	// Try keep this thread safe. Redundant work is OK, just don't die.
	if ( OSFuncTableStatus == 0 )
	{
		HMODULE kernel = GetModuleHandle( L"kernel32.dll" );
		AbcAssert( kernel );
		if ( GetProcAddress( kernel, "RtlInstallFunctionTableCallback" ) != NULL )
			OSFuncTableStatus = 1;
		else
			OSFuncTableStatus = -1;
	}
	return OSFuncTableStatus == 1;
}
#endif

void* JitPool::InsertFunction( const void* func, size_t funcBytes, const void* signature, JitUnwindInfo* unwind )
{
	// This is an open question. Right now I can't see why you would want to do this.
	ASSERT( !Sig2Func.contains( HashSig(signature, SignatureBytes) ) );

	if ( IsFull(funcBytes) )
	{
		Trim();
		if ( IsFull(funcBytes) )
		{
			ASSERT(false);
			return NULL;
		}
	}

	void* fpos = Pool + PoolPos;
	void* spos = GetSignature( Count );
	memcpy( fpos, func, funcBytes );
	memcpy( spos, signature, SignatureBytes );
	HashInsert( HashSig(spos, SignatureBytes), Count );
	Functions[Count].LastUsed = NextDate();
	Functions[Count].Pos = PoolPos;
	Functions[Count].Size = funcBytes;
	Functions[Count].OrgIndex = Count;
	if ( RFTable )
	{
#if JITPOOL_ENABLE_FUNC_TABLE
		ASSERT( unwind );
		RFTable[Count].BeginAddress = (DWORD) PoolPos;
		RFTable[Count].EndAddress = (DWORD) (PoolPos + funcBytes);
		RFTable[Count].UnwindData = (DWORD) (RFUnwind() - Pool + UnwindItemOffset(Count));
		MakeUnwindInfo( Count, unwind );
#endif
	}
	else
	{
		//ASSERT( !unwind );
	}
	PoolPos += funcBytes;
	Count++;
	return fpos;
}

// Older versions of the Windows SDK don't export these. I first saw these publicly exported at Windows SDK v8.0 (2013)
#ifndef UNW_FLAG_NHANDLER
#define UNW_FLAG_NHANDLER       0x0
#define UNW_FLAG_EHANDLER       0x1
#define UNW_FLAG_UHANDLER       0x2
#define UNW_FLAG_CHAININFO      0x4
#endif

void JitPool::MakeUnwindInfo( int ifunc, JitUnwindInfo* unwind )
{
#if JITPOOL_ENABLE_FUNC_TABLE
	if ( !unwind ) return;

	UNWIND_INFO* inf = (UNWIND_INFO*) (RFUnwind() + UnwindSize * ifunc);
	UNWIND_CODE* codes = (UNWIND_CODE*) (inf + 1);
	memset( codes, 0, sizeof(UNWIND_CODE) * MaxUnwindCodes );

	memcpy( inf, &unwind->W64Inf, sizeof(UNWIND_INFO) );
	memcpy( codes, &unwind->W64Codes, sizeof(UNWIND_CODE) * inf->CountOfCodes );


	/*
	inf->Version = 1;
	inf->Flags = 0;
	inf->SizeOfProlog = unwind->SizeOfProlog;
	inf->CountOfCodes = 0;

	if ( unwind->FrameRegister == REG_RAX ) ASSERT( unwind->FrameRegisterOffset == 0 );
	else
	{
		ASSERT( unwind->FrameRegisterOffset >= 16 );
		ASSERT( (unwind->FrameRegisterOffset & 15) == 0 );
	}
	inf->FrameRegister = Reg_To_UNWIND( unwind->FrameRegister );
	inf->FrameOffset = unwind->FrameRegisterOffset / 16;

	UNWIND_CODE* c = codes;
	int codePos = 0;

	// TODO: Add code that saves the 4 argument registers into their home locations.
	// HMMM: Surely that's not part of the this shebang?

	for ( uint i = 0; i < unwind->CountSavedReg; i++ )
	{
		// number of bytes of the relevant PUSH instruction
		int isize;
		if ( Reg_To_UNWIND(unwind->SavedReg[i]) >= 8 ) // 8 = R8. These push instructions are 2 bytes vs 1 for the rest of them.
			isize = 2;
		else
			isize = 1;

		c->CodeOffset = codePos + isize;
		c->UnwindOp = UWOP_PUSH_NONVOL;
		c->OpInfo = Reg_To_UNWIND(unwind->SavedReg[i]);
		codePos += isize;
		c++;
	}

	if ( unwind->TotalStackLocalBytes != 0 )
	{
		// AsmJit always emits 4 byte sub( rsp, xxx ) instructions
		c->CodeOffset = codePos + 4;
		c->UnwindOp = UWOP_ALLOC_SMALL;
		ASSERT( unwind->TotalStackLocalBytes >= 8 && (unwind->TotalStackLocalBytes & 7) == 0 );
		c->OpInfo = (unwind->StackLocalBytes - 8) / 8;
		codePos += 4;
		c++;
	}

	if ( unwind->FrameRegister != REG_RAX )
	{
		ASSERT(false); // instruction size please, for the lea()?
		codePos += 4;
	}

	for ( uint i = 0; i < unwind->CountSavedXmm; i++ )
	{
		// xmm8 and above are 7 byte instructions. xmm0 to xmm7 are 6 byte instructions (that is, movdqa)
		int isize = Reg_To_UNWIND(unwind->SavedXmm[i]) >= 8 ? 7 : 6;
		c->CodeOffset = codePos + isize;
		c->UnwindOp = UWOP_SAVE_XMM128;
		c->OpInfo = Reg_To_UNWIND(unwind->SavedXmm[i]);
		codePos += isize;
		c++;
	}

	inf->CountOfCodes = c - codes;
	ASSERT( inf->CountOfCodes <= MaxUnwindCodes );
	for ( uint i = 0; (int) i < inf->CountOfCodes / 2; i++ )
	{
		AbCore::Swap( codes[i], codes[inf->CountOfCodes - 1 - i] );
	}
	*/
#endif
}

#if JITPOOL_ENABLE_FUNC_TABLE
RUNTIME_FUNCTION* JitPool::DebugGetRuntimeFunction( void* f )
{
	return GetRuntimeFunction( (DWORD64) f, this );
}

UNWIND_INFO* JitPool::DebugGetUnwindInfo( void* f )
{
	RUNTIME_FUNCTION* rf = DebugGetRuntimeFunction( f );
	if ( !rf ) return NULL;
	return (UNWIND_INFO*) (Pool + rf->UnwindData);
}
#endif

void* JitPool::GetBySignature( const void* signature )
{
	int f = HashGet( HashSig(signature, SignatureBytes) );
	if ( f == NoFunc ) return NULL;
	Functions[f].LastUsed = NextDate();
	return Pool + Functions[f].Pos;
}

void* ZMalloc( size_t bytes )
{
	void* p = malloc(bytes);
	memset( p, 0, bytes );
	return p;
}

void JitPool::Trim()
{
	// Repack the most recently used half of the functions
	qsort( Functions, Count, sizeof(Func), &Func::Compare );

	TrimCount++;
	Count /= 2;

	BYTE* temp = (BYTE*) ZMalloc( PoolTotalBytes );
	BYTE* tcodebuf = temp;
	BYTE* tsig = (BYTE*) ZMalloc( SignatureBytes * Count );
#if JITPOOL_ENABLE_FUNC_TABLE
	BYTE* tuwbuf = UnwindPtr( tcodebuf, PoolSize );
	RUNTIME_FUNCTION* trftable = RFTable ? (RUNTIME_FUNCTION*) ZMalloc( sizeof(RUNTIME_FUNCTION) * Count ) : NULL;
#else
	BYTE* tuwbuf = NULL;
	void* trftable = NULL;
#endif

	for ( int i = 0; i < Count; i++ )
	{
		int iOrg = Functions[i].OrgIndex;
		memcpy( tcodebuf, Pool + Functions[i].Pos, Functions[i].Size );
		memcpy( tsig + i * SignatureBytes, SigPool + iOrg * SignatureBytes, SignatureBytes );
		if ( trftable )
		{
#if JITPOOL_ENABLE_FUNC_TABLE
			memcpy( tuwbuf + UnwindItemOffset(i), RFUnwind() + UnwindItemOffset(iOrg), UnwindSize );
			//memcpy( trftable + i, RFTable + iOrg, sizeof(RUNTIME_FUNCTION) );
			trftable[i].BeginAddress = tcodebuf - temp;
			trftable[i].EndAddress = trftable[i].BeginAddress + (DWORD) Functions[i].Size;
			trftable[i].UnwindData = (DWORD) UnwindDataStart(PoolSize) + UnwindItemOffset(i);
#endif
		}
		Functions[i].LastUsed = Count - i;
		Functions[i].Pos = tcodebuf - temp;
		Functions[i].OrgIndex = i;
		tcodebuf += Functions[i].Size;
	}

	size_t bytes = tcodebuf - temp;
	PoolPos = bytes;

	memcpy( Pool, temp, PoolTotalBytes );
	memcpy( SigPool, tsig, SignatureBytes * Count );
#if JITPOOL_ENABLE_FUNC_TABLE
	if ( RFTable ) memcpy( RFTable, trftable, sizeof(RUNTIME_FUNCTION) * Count );
#endif
	free( temp );
	free( tsig );
	free( trftable );

	Sig2Func.clear();
	for ( int i = 0; i < Count; i++ )
		HashInsert( HashSig(GetSignature(i), SignatureBytes), i );

	Date = Count + 1;
}

void JitPool::TrimEnsureSpaceFor( int nFunctions, size_t nTotalBytes )
{
	ASSERT( (size_t) nFunctions < MaxElements / 2 );
	ASSERT( nTotalBytes < PoolTotalBytes / 2 );
	if ( Count + (size_t) nFunctions > MaxElements || PoolPos + nTotalBytes > PoolSize )
	{
		Trim();
	}
}

}
