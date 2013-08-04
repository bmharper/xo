#pragma once

#include "lmTypes.h"

DISABLE_CODE_ANALYSIS_WARNINGS_PUSH_ALL
#include <AsmJit/AsmJit/AsmJit.h>
DISABLE_CODE_ANALYSIS_WARNINGS_POP

typedef unsigned char UBYTE;
typedef unsigned int uint;

namespace AsmJit
{


/** Pool of JIT'ed functions.

I created this because allocating 4k for every function that you JIT could quickly be wasteful.

Every function that you insert has an associated signature. You use that signature to find the function.
It is your responsibility to ensure that no two functions have the same signature.

When the pool is full and you attempt an insert, it does the following:
* Removes half of the functions. The oldest half is removed.

**/ 
class PAPI JitPool
{
public:
	JitPool();
	~JitPool();

	static const size_t PageSize = 4096;

	// There are 7 non-volatile registers. Plus 2 instructions max for stack space = 9 codes. Plus 20 for all 10 XMM registers = 29. 30 seems like enough.
	static const int		MaxUnwindCodes = 30;
	static const int		UnwindSize = sizeof(UNWIND_INFO) + MaxUnwindCodes * sizeof(UNWIND_CODE);

	struct Func
	{
		size_t Pos;		///< Position relative to start of code block
		size_t Size;	///< Not normally a necessary field, but makes sorting the array much easier
		int LastUsed;
		int OrgIndex;	///< Used only when sorting

		static int Compare( const void* a, const void* b )
		{
			Func* fa = (Func*) a;
			Func* fb = (Func*) b;
			// We want the newest items to be first
			return fb->LastUsed - fa->LastUsed;
		}
	};

	/// Reset the pool, clearing all data.
	void Reset();

	/// Round up to the nearest page size
	static size_t RoundUpToPageSize( size_t v )
	{
		return (v + PageSize - 1) & ~(PageSize - 1);
	}

	/** Initialize the pool.
	@param poolBytes The total number of bytes reserved for code.
		It is your responsibility to round this value to up the nearest page size.
		The static function RoundUpToPageSize() is provided to make that trivial. Obviously the VirtualAlloc does this anyway,
		but the reason we allow arbitrary values is to make it easy to test the overflow mechanisms of the pool. With all of this in mind,
		note also that upon entry to this function, we do round poolBytes up to the nearest 16 bytes.
	@param maxElements The maximum number of functions.
	@param signatureSize The size of function signatures. If you need variable sizes,
		then make this the maximum size you will need, and pad smaller signatures with a fixed value.
	@param enableFuncTable64 Only when this option is true, do we do the whole RtlInstallFunctionTableCallback thing.
	**/
	void Initialize( size_t poolBytes, size_t maxElements, size_t signatureSize, bool enableFuncTable64 );

	/** Register a new function.
	@return The address of the new function in the executable block.
	**/
	void* InsertFunction( const void* func, size_t funcBytes, const void* signature, JitUnwindInfo* unwind );

	/** Retrieve a function based on its signature.
	@return The function address, or NULL if the function does not exist.
	**/
	void* GetBySignature( const void* signature );

	/** Removes half of the functions.
	This is automatically called by InsertFunction(), when necessary. It is exposed for testing.
	**/
	void Trim();
	
	/** Trims the pool if necessary, to ensure that there is space for x number of functions and y number of total code bytes. 
	This was created for Baboon, my software renderer, which creates about eight functions in one go. The problem is, if a Trim
	happens while you're generating function number three, then functions one and two are invalidated, and you wouldn't realize it.
	**/
	void TrimEnsureSpaceFor( int nFunctions, size_t nTotalBytes );

	/// Returns the number of registered functions.
	int GetCount() { return Count; }

	/// Returns the number of times that Trim() has been called, since Reset() was last called.
	int GetTrimCount() { return TrimCount; }

	/// Returns the number of bytes used for code.
	size_t GetCodeSize() { return PoolPos; }

	/// Returns the base address of the functions.
	void* GetCodeBase() { return Pool; }

	/// Returns the number of bytes allocated to the pool
	size_t GetCodePoolBytes() { return PoolSize; }

#if defined(_MSC_VER) && defined(_M_X64)
	RUNTIME_FUNCTION* DebugGetRuntimeFunction( void* f );
	UNWIND_INFO* DebugGetUnwindInfo( void* f );
#endif

protected:

	size_t					MaxElements;		///< Maximum number of functions registered
	size_t					SignatureBytes;		///< Size of one signature
	HashSigIntMap			Sig2Func;
	size_t					PoolSize;			///< Requested size of data allocated to Pool
	size_t					PoolTotalBytes;		///< Total size of data allocated to Pool (rounded to allocation granularity size)
	size_t					PoolPos;
	BYTE*					Pool;				///< The code pool. Requested size in bytes is PoolSize. Total size of allocated block is PoolTotalBytes.
	BYTE*					SigPool;			///< All the signatures, in a contiguous array (must be constant, since the hash table encodes pointers into this array)
	Func*					Functions;
#if defined(_MSC_VER) && defined(_M_X64)
	RUNTIME_FUNCTION*		RFTable;
	static size_t			UnwindDataStart( size_t poolSize ) { return poolSize; }
	static BYTE*			UnwindPtr( void* poolBase, size_t poolSize ) { return (BYTE*) poolBase + UnwindDataStart(poolSize); }
	static DWORD			UnwindItemOffset( int i ) { return i * UnwindSize; }
	BYTE*					RFUnwind() { return UnwindPtr(Pool, PoolSize); }
	bool					OSHasFuncTable();
#else
	void*					RFTable;
#endif
	bool					RFEnable;
	bool					RFInstalled;
	int						Date;
	int						Count;
	int						TrimCount;


	static const int		FPosOffset = 1;
	static const int		NoFunc = 0 - FPosOffset;
	
	bool IsFull( size_t forFunctionBytes )
	{
		return PoolPos + forFunctionBytes > PoolSize || Count == MaxElements;
	}

	void	HashInsert( const HashSig& s, int v )	{ Sig2Func.insert( s, v + FPosOffset, true ); } // true => overwrite existing function with this signature
	int		HashGet( const HashSig& s )				{ return Sig2Func.get( s ) - FPosOffset; }

	void MakeUnwindInfo( int ifunc, JitUnwindInfo* unwind );

#if defined(_MSC_VER) && defined(_M_X64)
	static PRUNTIME_FUNCTION GetRuntimeFunction( DWORD64 controlPC, PVOID context );
#endif

	void* GetSignature( int f ) { return SigPool + f * SignatureBytes; }
	int NextDate()
	{
		if ( Date == INT32MAX ) Date = 0; // Wrap around. Not 100% proper behaviour, but it will only cause invalid heuristics.
		return Date++;
	}
};

}
