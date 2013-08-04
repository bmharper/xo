/*
*  sha1.h
*
*	Copyright (C) 1998
*	Paul E. Jones <paulej@arid.us>
*	All Rights Reserved
*
*****************************************************************************
*	$Id: sha1.h,v 1.2 2004/03/27 18:00:33 paulej Exp $
*****************************************************************************
*
*  Description:
*      This class implements the Secure Hashing Standard as defined
*      in FIPS PUB 180-1 published April 17, 1995.
*
*      Many of the variable names in the SHA1Context, especially the
*      single character names, were used because those were the names
*      used in the publication.
*
*      Please read the file sha1.c for more information.
*
*/

#ifndef _SHA1_H_
#define _SHA1_H_

/* 
*  This structure will hold context information for the hashing
*  operation
*/
typedef struct SHA1Context
{
		unsigned Message_Digest[5]; /* Message Digest (output)          */

		unsigned Length_Low;        /* Message length in bits           */
		unsigned Length_High;       /* Message length in bits           */

		unsigned char Message_Block[64]; /* 512-bit message blocks      */
		int Message_Block_Index;    /* Index into message block array   */

		int Computed;               /* Is the digest computed?          */
		int Corrupted;              /* Is the message digest corruped?  */
} SHA1Context;

#ifdef __cplusplus
extern "C" 
{
#endif

/*
*  Function Prototypes
*/
void SHA1Reset(SHA1Context *);
int SHA1Result(SHA1Context *);
void SHA1Input( SHA1Context *,
								const unsigned char *,
								unsigned);

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#ifdef __cplusplus
namespace AbCore
{
	class PAPI SHA1 : public IAppendableHash
	{
	public:
		static const u32 DBYTES = 20;

		SHA1Context state;

		SHA1()													{ Reset(0); }
		virtual void	Reset( u32 seed )							{ SHA1Reset( &state ); }
		virtual void	Append( const void* data, size_t bytes )	{ SHA1Input( &state, (const unsigned char*) data, (unsigned int) bytes ); }
		virtual void	Finish( void* output )						{ SHA1Result( &state ); CopyOut( state, output ); }
		virtual u32		DigestBytes()								{ return DBYTES; }

		static void Compute( void* output, const void* data, size_t bytes )
		{
			SHA1 t;
			t.Append( data, bytes );
			t.Finish( output );
		}

		static bool Check( const void* digest, const void* data, size_t bytes )
		{
			u8 d[DBYTES];
			Compute( d, data, bytes );
			return CryptoMemCmpEq<DBYTES>(digest, d);
		}

	protected:
		static void CopyOut( const SHA1Context& cx, void* digest )
		{
			// need to correct for Intel Endianness
			BYTE* bout = (BYTE*) digest;
			for ( int ob = 0; ob < DBYTES; ob++ )
			{
				BYTE* bin = (BYTE*) &cx.Message_Digest[ ob / 4 ];
				bout[ob] = bin[ 3 - ob % 4 ];
			}
		}
	};
}
#endif


#endif
