#pragma once

#include "../Other/lmDefs.h"

namespace AbCore
{
	/** Internal function used to update CRC32 computation.
	We keep it internal (ie non-inline) so that the lookup table does not pollute.
	The returned value must be binary negated before use.
	**/
	UINT32 PAPI crc32_append( UINT crc, const void* buf, size_t bytes );

	inline UINT32 crc32_init()
	{
		return (UINT32) ~ 0;
	}

	/** Calculate CRC32, reportedly used in PKZip, AUTODIN II, Ethernet, and FDDI.
	This uses code by Spencer Garrett <srg@quick.com>, taken out of AGG's agg_font_freetype.cpp,
	which uses a lookup table of 1024 bytes.
	**/
	inline UINT32 crc32_compute( const void* buf, size_t bytes )
	{
		return ~ crc32_append( crc32_init(), buf, bytes );
	}

	class PAPI CRC32 : public IAppendableHash
	{
	public:
		UINT32 icrc;

		CRC32()														{ Reset(0); }
		virtual void	Reset( u32 seed )							{ icrc = ~seed; }
		virtual void	Append( const void* data, size_t bytes )	{ icrc = crc32_append( icrc, data, bytes ); }
		virtual void	Finish( void* output )						{ u32 res = ~icrc; memcpy( output, &res, sizeof(res) ); }
		virtual u32		DigestBytes()								{ return 4; }

		static u32 Compute( const void* data, size_t bytes )
		{
			return crc32_compute( data, bytes );
		}
	};

	class PAPI Adler32 : public IAppendableHash
	{
	public:
		UINT32 adler;

		Adler32()													{ Reset(0); }
		virtual void	Reset( u32 seed );
		virtual void	Append( const void* data, size_t bytes );
		virtual void	Finish( void* output )						{ memcpy( output, &adler, sizeof(adler) ); }
		virtual u32		DigestBytes()								{ return 4; }

		static u32 Compute( const void* data, size_t bytes );

	};

	class PAPI Murmur2 : public INonAppendableHash
	{
	public:
		virtual void	Compute( const void* input, size_t bytes, u32 seed, void* output )	{ u32 res = MurmurHash2( input, (int) bytes, seed ); memcpy(output, &res, sizeof(res)); }
		virtual u32		DigestBytes()														{ return 4; }
	};

	class PAPI Murmur3_x86_32 : public INonAppendableHash
	{
	public:
		virtual void	Compute( const void* input, size_t bytes, u32 seed, void* output )	{ MurmurHash3_x86_32( input, (int) bytes, seed, output ); }
		virtual u32		DigestBytes()														{ return 4; }
	};

	class PAPI Murmur3_x86_128 : public INonAppendableHash
	{
	public:
		virtual void	Compute( const void* input, size_t bytes, u32 seed, void* output )	{ MurmurHash3_x86_128( input, (int) bytes, seed, output ); }
		virtual u32		DigestBytes()														{ return 16; }
	};

}
