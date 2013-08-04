#pragma once

#include "../fhash/fhashtable.h"
#include "../Other/lmTypes.h"

struct PAPI DataSig8
{
	union
	{
		u8	Bytes[8];
		u32 DWords[2];
		u64 QWord;
	};

	static DataSig8 Null() { DataSig8 s; s.SetNull(); return s; }

	bool operator==( const DataSig8& b ) const { return QWord == b.QWord; }
	bool operator!=( const DataSig8& b ) const { return !(*this == b); }

	bool IsNull() const { return QWord == 0; }
	void SetNull()		{ QWord = 0; }

	static DataSig8 GenerateFromTime();
	static DataSig8 GenerateFromFileAttribs( const XStringW& filename, uint seed );

};

struct PAPI DataSig20
{
	static const unsigned int DBYTES = 20;
	union
	{
		u8	Bytes[DBYTES];
		u32 DWords[DBYTES/4];
	};

	static DataSig20 Null() { DataSig20 s; s.SetNull(); return s; }

	bool operator==( const DataSig20& b ) const { return CryptoMemCmpEq(Bytes, b.Bytes, DBYTES); }
	bool operator!=( const DataSig20& b ) const { return !CryptoMemCmpEq(Bytes, b.Bytes, DBYTES); }

	bool IsNull() const { return 0 == (DWords[0] | DWords[1] | DWords[2] | DWords[3] | DWords[4]); }
	void SetNull()		{ DWords[0] = DWords[1] = DWords[2] = DWords[3] = DWords[4] = 0; }

	static DataSig20 CreateFromSHA1( const void* data, size_t bytes );
	void SetFromSHA1( const void* data, size_t bytes );

};

namespace AbCore
{

	/** Signature used to uniquely identify something. Used by adb, Albion, AbGis.
	
	NOTE: If you change this to 20 bytes some day, make sure you pack() it so that its sizeof() is indeed 20 bytes.
	
	NO! If you change it to 20, make a new structure called DataSig20.

	**/
	struct PAPI DataSig
	{
		static const int NumBytes = 16;
		union
		{
			uint8 Bytes[NumBytes];
			uint32 DWords[NumBytes / 4];
		};

		// Useful when you don't want to refer to a variable's name, such as for(...) insert(data[i], data[i].ByteCount())
		int ByteCount() const { return NumBytes; }

		void SetNull() { memset( this, 0, sizeof(*this) ); }
		bool IsNull() const { return (DWords[0] | DWords[1] | DWords[2] | DWords[3]) == 0; }

		/// Typically used in the form ASSERT( !sig.DebugAnyWordsMatch( 0xCDCDCDCD ) );
		bool DebugAnyWordsMatch( UINT32 val ) const
		{
			return DWords[0] == val || DWords[1] == val || DWords[2] == val || DWords[3] == val;
		}

		// By incrementing two words we reduce the chance of collisions
		void Inc()	{ DWords[2]++; DWords[3]++; }

		bool operator==( const DataSig& b ) const { return memcmp(this, &b, sizeof(*this)) == 0; }
		bool operator!=( const DataSig& b ) const { return !(*this == b); }

		Guid ToGuid() const { Guid g; memcpy( &g, this, 16 ); return g; }

		int GetHashCode() const
		{
			return DWords[0] ^ DWords[1] ^ DWords[2] ^ DWords[3];
		}

		static const int HexCharsZ = NumBytes * 2 + 1;			///< Without separator
		static const int HexCharsSepZ = NumBytes * 2 + 3 + 1;	///< With separator

		wchar_t*	HexSignature( wchar_t* sig, wchar_t quad_splitter, bool nullTerm = false ) const;
		char*		HexSignature( char* sig, char quad_splitter, bool nullTerm = false ) const;
		template< typename TCH >
		TCH*		HexSignatureT( TCH* sig, TCH quad_splitter, bool nullTerm ) const;

		template< typename TStr, typename TChar >
		TStr THexSignature_R( wchar_t quad_splitter ) const
		{
			TChar buf[HexCharsSepZ];
			HexSignature( buf, quad_splitter, true );
			return buf;
		}
		XStringA HexSignatureA( char quad_splitter ) const { return THexSignature_R<XStringA, char>( quad_splitter ); }
		XStringW HexSignatureW( char quad_splitter ) const { return THexSignature_R<XStringW, wchar_t>( quad_splitter ); }

		void Mixin( const DataSig& sig )	{ *this = Combine( *this, sig ); }
		void Mixin( uint32 sig32 )			{ *this = Combine( *this, sig32 ); }
		void Mixin64( uint64 sig64 )		{ *this = Combine64( *this, sig64 ); }
		void MixinXor( const DataSig& sig )
		{
			DWords[0] ^= sig.DWords[0];
			DWords[1] ^= sig.DWords[1];
			DWords[2] ^= sig.DWords[2];
			DWords[3] ^= sig.DWords[3];
		}

		void Mixin( const void* data, int bytes )	{ *this = Combine( *this, data, bytes ); }

		// beware of non-packed structs!
		template< typename T>
		void TMixin( const T& val )			{ Mixin( &val, sizeof(val) ); }

		void Update( const void* data, int bytes )
		{
			*this = Generate( data, bytes );
		}

		/// Generate a DataSig from the file last write time and size.
		static DataSig GenerateFromFileAttribs( const XStringW& filename, uint mixin = 0 );

		// This is really just a type cast from a GUID to a DataSig
		// If in doubt, DO NOT use this, but instead use FromHashedGuid()
		static DataSig FromGuid( const Guid& g ) { DataSig s; memcpy( &s, &g, 16 ); return s; }

		// This first hashes the GUID, and then returns a new DataSig
		// If you are using the OS to generate GUIDs, and you are using those for entropy, then you must
		// use this function instead of "FromGuid". We had a bug once where we would generate two signatures
		// in quite different parts of the application, and then we would XOR them together. It turns out
		// that if those two GUIDs were generated on the same machine, at roughly the same time, then
		// the XOR would cancel out almost all of the entropy present in the original GUID.
		static DataSig FromHashedGuid( const Guid& g ) { return DataSig::GenerateT(g); }

		static DataSig Null() { DataSig s; s.SetNull(); return s; }

		template< typename T >
		static DataSig GenerateT( const T& t )
		{
			return Generate( &t, sizeof(t) );
		}

		static DataSig Generate( const void* data, size_t bytes );

		/// Generate from null terminated string
		template< typename TCH >
		static DataSig GenerateFromStringZ( const TCH* data )
		{
			int len = 0;
			while ( data[len] ) len++;
			return Generate( data, sizeof(data[0]) * len );
		}

		static DataSig Combine( DataSig a, DataSig b )
		{
			DataSig both[2] = {a, b};
			return Generate( both, sizeof(both) );
		}

		// beware of non-packed structs!
		template< typename T >
		static DataSig TCombine( DataSig a, T extra )
		{
			BYTE buff[NumBytes + sizeof(extra)];
			memcpy( buff, &a, NumBytes );
			memcpy( buff + NumBytes, &extra, sizeof(extra) );
			return Generate( buff, sizeof(buff) );
		}

		static DataSig Combine( DataSig a, const void* data, int bytes )
		{
			DataSig b = Generate( data, bytes );
			return Combine( a, b );
		}

		static DataSig Combine( DataSig a, uint32 i )
		{
			return TCombine( a, i );
		}

		static DataSig Combine64( DataSig a, uint64 i )
		{
			return TCombine( a, i );
		}

	};

}
FHASH_SETUP_POD_GETHASHCODE(AbCore::DataSig);

typedef AbCore::DataSig DataSig16;

