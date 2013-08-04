#pragma once

namespace AbCore
{
	class IFile;

	struct CSVContext
	{
		CSVContext()
		{
			Delim = ',';
			Quote = '"';
			MaxColumns = 0;
		}
		wchar_t Delim;
		wchar_t Quote;
		INT32 MaxColumns;
	};

	template< typename TCH >
	struct CSVTokenizer
	{
		TCH* DstBuf;
		INT32* Positions;
		INT32 Pos;
		CSVTokenizer( TCH* dstBuf, INT32* positions )
		{
			DstBuf = dstBuf;
			Positions = positions;
			Pos = 0;
		}

		TCH* Cur() { return DstBuf + Positions[Pos]; }
		INT32 Len() { return Positions[Pos + 1] - Positions[Pos]; }

		CSVTokenizer& operator++(int) { Pos++; return *this; }
		CSVTokenizer& operator++()		{ Pos++; return *this; }

	};

	enum CSVStatus
	{
		CSVWriteOk = 0,
		CSVDestBufferOverflow = -1,
		CSVDestPositionsOverflow = -2,
		CSVUnterminatedQuote = -3,
	};

	/** Read a line of a CSV (comma separated value) file.
	@param context The CSV context.
	@param srcLen The length of the source string, or -1 if it is null terminated.
	@param srcBuf The source string.
	@param dstLen The length of the destination buffer.
	@param dstBuf The destination buffer.
	@param positions An array of starting character index for each token. There will be an additional terminating entry at the end
		which points to one character beyond the last character of the last token. Thus, there must be space for
		CSVContext.MaxColumns + 1 entries in the positions array.
	@return The number of tokens read, or a CSVStatus error code if there was an error.
	**/
	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const wchar_t* srcBuf, INT32 dstLen, wchar_t* dstBuf, INT32* positions );

	/// Single byte version of CSVReadLine (makes no attempt at UTF interpretation).
	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const char* srcBuf, INT32 dstLen, char* dstBuf, INT32* positions );

#if XSTRING_DEFINED
	INT32 PAPI CSVReadLine( const CSVContext& context, const XStringW& src, podvec<XStringW>& result );
	INT32 PAPI CSVReadLine( const CSVContext& context, const XStringA& src, podvec<XStringA>& result );
	INT32 PAPI CSVReadLine( const XStringW& src, podvec<XStringW>& result );
	INT32 PAPI CSVReadLine( const XStringA& src, podvec<XStringA>& result );
	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const wchar_t* srcBuf, podvec<XStringW>& result );
	INT32 PAPI CSVReadLine( const CSVContext& context, INT32 srcLen, const char* srcBuf, podvec<XStringA>& result );
#endif

	/** Write a line of a CSV (comma separated value) file.
	@return CSVWriteOk, or CSVDestBufferOverflow if the destination buffer was not large enough.
	**/
	INT32 PAPI CSVWriteLine( const CSVContext& context, const wchar_t* srcBuf, INT32& dstLen, wchar_t* dstBuf, INT32 nTokens, const INT32* positions );

	/// Single byte version of CSVWriteLine (makes no attempt at UTF interpretation).
	INT32 PAPI CSVWriteLine( const CSVContext& context, const char* srcBuf, INT32& dstLen, char* dstBuf, INT32 nTokens, const INT32* positions );

	/// Single byte version of CSVWriteLine (makes no attempt at UTF interpretation).
	INT32 PAPI CSVWriteLine( const CSVContext& context, const char* srcBuf, AbCore::IFile* dstFile, INT32 nTokens, const INT32* positions, INT32 tempBufferLen = 0, char* tempBuffer = NULL );

	/** Pack a vector of strings into a single line, ready for writing.
	This is obviously rather slow.
	**/
	bool PAPI CSVPack( const dvect<XStringW>& src, INT32& dstLen, wchar_t* dstBuf, dvect<INT32>& positions );


}
