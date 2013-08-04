#include "pch.h"
#include "CSV.h"
#include "IO/IFile.h"


namespace AbCore
{
	template< typename RCHAR >
	INT32 CSVReadLineT( const CSVContext& context, INT32 srcLen, const RCHAR* srcBuf, INT32 dstLen, RCHAR* dstBuf, INT32* positions )
	{
		if ( srcLen == 0 || srcBuf == NULL || *srcBuf == 0 ) return 0;
		if ( context.MaxColumns <= 0 ) return 0;
		INT32 slots = 0;
		positions[slots] = 0;
		const RCHAR* src = srcBuf;
		const RCHAR* srcEnd = srcBuf + srcLen;
		RCHAR* dst = dstBuf;
		RCHAR* dstEnd = dstBuf + dstLen;
		bool quoted = false;
		bool atSrcEnd = false;
		INT32 tokLen = 0;
		INT32 skip = 0;
		for ( ; !atSrcEnd; src++, tokLen++ )
		{
			atSrcEnd = src == srcEnd || *src == 0;
			RCHAR ch = atSrcEnd ? context.Delim : *src;
			if ( skip > 0 ) { skip--; continue; }
			if ( tokLen == 0 )
			{
				// if first character is a quote, then expect a quote to terminate the token
				quoted = ch == context.Quote;
				if ( quoted ) continue;
			}
			if ( quoted )
			{
				if ( ch == context.Quote )
				{
					RCHAR chNext = (!atSrcEnd && src + 1 != srcEnd) ? *(src + 1) : 0;
					if ( chNext == context.Quote )
					{
						// double-quote => outputs a single quote
						skip = 1;
					}
					else
					{
						// quote is terminated
						quoted = false;
						continue;
					}
				}
			}
			else if ( ch == context.Delim )
			{
				// delimiter
				if ( slots == context.MaxColumns ) return CSVDestPositionsOverflow;
				slots++;
				positions[slots] = dst - dstBuf;
				tokLen = -1;
				continue;
			}

			if ( dst == dstEnd ) return CSVDestBufferOverflow;
			else
			{
				// Add a character to destination buffer
				*dst++ = ch;
			}
		}
		if ( quoted ) return CSVUnterminatedQuote;
		return slots;
	}

	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const wchar_t* srcBuf, INT32 dstLen, wchar_t* dstBuf, INT32* positions )
	{
		return CSVReadLineT( context, srcLen, srcBuf, dstLen, dstBuf, positions );
	}

	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const char* srcBuf, INT32 dstLen, char* dstBuf, INT32* positions )
	{
		return CSVReadLineT( context, srcLen, srcBuf, dstLen, dstBuf, positions );
	}

	template<typename TCH, typename TStr>
	INT32 PAPI CSVReadLineT_V( const CSVContext& context, INT32 srcLen, const TCH* srcBuf, podvec<TStr>& result )
	{
		INT32 dstLen = srcLen * sizeof(TCH);
		TCH* dstBuf = (TCH*) AbcMallocOrDie( dstLen );
		INT32* pos = (INT32*) AbcMallocOrDie( (context.MaxColumns + 1) * sizeof(INT32) );
		INT32 n = CSVReadLine( context, srcLen, srcBuf, dstLen, dstBuf, pos );
		if ( n > 0 )
		{
			for ( INT32 i = 0; i < n; i++ )
				result.push( TStr(&dstBuf[pos[i]], pos[i + 1] - pos[i]) );
		}
		free(dstBuf);
		free(pos);
		return n;
	}

	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const wchar_t* srcBuf, podvec<XStringW>& result )
	{
		return CSVReadLineT_V( context, srcLen, srcBuf, result );
	}

	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const char* srcBuf, podvec<XStringA>& result )
	{
		return CSVReadLineT_V( context, srcLen, srcBuf, result );
	}

	INT32 PAPI CSVReadLine( const CSVContext& context, const XStringW& src, podvec<XStringW>& result )
	{
		return CSVReadLine( context, src.Length(), src, result );
	}

	INT32 PAPI CSVReadLine( const CSVContext& context, const XStringA& src, podvec<XStringA>& result )
	{
		return CSVReadLine( context, src.Length(), src, result );
	}

	template<typename TCH, typename TStr>
	INT32 PAPI CSVReadLineT_2( const TStr& src, podvec<TStr>& result )
	{
		CSVContext cx;
		cx.MaxColumns++;
		const TCH comma = ',';
		for ( int i = 0; i < src.Length(); i++ )
			cx.MaxColumns += src[i] == comma ? 1 : 0;
		return CSVReadLine( cx, src, result );
	}

	INT32 PAPI CSVReadLine( const XStringW& src, podvec<XStringW>& result )
	{
		return CSVReadLineT_2<wchar_t, XStringW>( src, result );
	}

	INT32 PAPI CSVReadLine( const XStringA& src, podvec<XStringA>& result )
	{
		return CSVReadLineT_2<char, XStringA>( src, result );
	}

	template< typename RCHAR >
	bool AddCh( RCHAR*& dst, RCHAR* dstEnd, RCHAR ch )
	{
		if ( dst == dstEnd ) return false;
		*dst++ = ch;
		return true;
	}

	template< typename RCHAR >
	INT32 CSVWriteLineT( const CSVContext& context, const RCHAR* srcBuf, INT32& dstLen, RCHAR* dstBuf, INT32 nTokens, const INT32* positions )
	{
		const RCHAR* src = srcBuf;
		RCHAR* dst = dstBuf;
		RCHAR* dstEnd = dstBuf + dstLen;
		for ( INT32 tok = 0; tok < nTokens; tok++ )
		{
			INT32 start = positions[tok];
			INT32 end = positions[tok + 1];

			if ( tok != 0 )
			{
				if ( !AddCh<RCHAR>( dst, dstEnd, context.Delim ) ) return CSVDestBufferOverflow;
			}

			// must first scan to see whether we need to quote this token
			bool quote = false;
			for ( INT32 scan = start; scan != end; scan++ )
			{
				if ( src[scan] == context.Delim || src[scan] == context.Quote )
				{
					quote = true;
					break;
				}
			}

			if ( quote && !AddCh<RCHAR>(dst, dstEnd, context.Quote) ) return CSVDestBufferOverflow;

			for ( INT32 scan = start; scan != end; scan++ )
			{
				if ( src[scan] == context.Quote )
				{
					// double quote
					if ( !AddCh<RCHAR>( dst, dstEnd, context.Quote ) ) return CSVDestBufferOverflow;
					if ( !AddCh<RCHAR>( dst, dstEnd, context.Quote ) ) return CSVDestBufferOverflow;
				}
				else
				{
					if ( !AddCh<RCHAR>( dst, dstEnd, src[scan] ) ) return CSVDestBufferOverflow;
				}
			}

			if ( quote && !AddCh<RCHAR>(dst, dstEnd, context.Quote) ) return CSVDestBufferOverflow;
		}
		dstLen = dst - dstBuf;
		return CSVWriteOk; 

	}

	INT32 PAPI CSVWriteLine( const CSVContext& context, const wchar_t* srcBuf, INT32& dstLen, wchar_t* dstBuf, INT32 nTokens, const INT32* positions )
	{
		return CSVWriteLineT( context, srcBuf, dstLen, dstBuf, nTokens, positions );
	}

	INT32 PAPI CSVWriteLine( const CSVContext& context, const char* srcBuf, INT32& dstLen, char* dstBuf, INT32 nTokens, const INT32* positions )
	{
		return CSVWriteLineT( context, srcBuf, dstLen, dstBuf, nTokens, positions );
	}

	INT32 PAPI CSVWriteLine( const CSVContext& context, const char* srcBuf, AbCore::IFile* dstFile, INT32 nTokens, const INT32* positions, INT32 tempBufferLen, char* tempBuffer )
	{
		INT32 dstLen = tempBufferLen;
		char* dstBuf = tempBuffer;
		char tb[1024];
		if ( tempBufferLen == 0 || dstBuf == NULL )
		{
			dstLen = sizeof(tb) / sizeof(tb[0]);
			dstBuf = tb;
		}

		while ( CSVWriteLine( context, srcBuf, dstLen, dstBuf, nTokens, positions ) == CSVDestBufferOverflow )
		{
			if ( dstBuf != tb && dstBuf != tempBuffer ) free(dstBuf);
			dstLen *= 2;
			dstBuf = (char*) malloc( dstLen );
		}

		// ignore result of IFile write
		dstFile->Write( dstBuf, dstLen );
		
		// I'd rather write 'UNIX' files, but Excel 2007 on a Vista PC requires CSV files to have PC Line Endings.
		// AHEM not true. Presence of ID field as first field is what does this.
		dstFile->WriteInt8( '\r' );
		dstFile->WriteInt8( '\n' );

		if ( dstBuf != tb && dstBuf != tempBuffer ) free(dstBuf);

		return CSVWriteOk;
	}

	bool PAPI CSVPack( const dvect<XStringW>& src, INT32& dstLen, wchar_t* dstBuf, dvect<INT32>& positions )
	{
		INT32 dpos = 0;
		positions += 0;
		bool bad = false;
		for ( INT32 i = 0; i < src.size(); i++ )
		{
			INT32 len = src[i].Length();
			if ( dpos + len > dstLen )
				bad = true;
			if ( !bad )
				memcpy( dstBuf + dpos, (const wchar_t*) src[i], len * sizeof(wchar_t) );
			dpos += len;
			positions += dpos;
		}
		dstLen = dpos;
		return !bad;
	}

}
