#include "pch.h"
#include "../disable_all_code_analysis_warnings.h" // Just assuming this stuff is good

/*
* Copyright 2001 Unicode, Inc.
* 
* Disclaimer
* 
* This source code is provided as is by Unicode, Inc. No claims are
* made as to fitness for any particular purpose. No warranties of any
* kind are expressed or implied. The recipient agrees to determine
* applicability of information provided. If this file has been
* purchased on magnetic or optical media from Unicode, Inc., the
* sole remedy for any claim will be exchange of defective media
* within 90 days of receipt.
* 
* Limitations on Rights to Redistribute This Code
* 
* Unicode, Inc. hereby grants the right to freely use the information
* supplied in this file in the creation of products supporting the
* Unicode Standard, and to make copies of this file in any form
* for internal or external distribution as long as this notice
* remains attached.
*/

/* ---------------------------------------------------------------------

	Conversions between UTF32, UTF-16, and UTF-8. Source code file.
	Author: Mark E. Davis, 1994.
	Rev History: Rick McGowan, fixes & updates May 2001.
	Sept 2001: fixed const & error conditions per
		mods suggested by S. Parent & A. Lillich.

		See the header file "ConvertUTF.h" for complete documentation.

------------------------------------------------------------------------ */


#include "ConvertUTF.h"

#ifdef CVTUTF_DEBUG
#include <stdio.h>
#endif

namespace Unicode
{

static const int halfShift	= 10; /* used for shifting by 10 bits */

static const UTF32 halfBase	= 0x0010000UL;
static const UTF32 halfMask	= 0x3FFUL;

#define UNI_SUR_HIGH_START		(UTF32) 0xD800
#define UNI_SUR_HIGH_END		(UTF32) 0xDBFF
#define UNI_SUR_LOW_START		(UTF32) 0xDC00
#define UNI_SUR_LOW_END			(UTF32) 0xDFFF

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF32toUTF16 (
		const UTF32** sourceStart, const UTF32* sourceEnd, 
		UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags) 
{
	ConversionResult result = ConversionOk;
	const UTF32* source = *sourceStart;
	UTF16* target = *targetStart;
	while (source < sourceEnd) 
	{
		UTF32 ch;
		if (target >= targetEnd) 
		{
			result = ConversionResultTargetExhausted; break;
		}
		ch = *source++;
		if (ch <= UNI_MAX_BMP) 
		{ 
			/* Target is a character <= 0xFFFF */
			if ((flags == ConversionStrict) && (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)) 
			{
				--source; /* return to the illegal value itself */
				result = ConversionResultSourceIllegal;
				break;
			} 
			else 
			{
				*target++ = ch;	/* normal case */
			}
		} 
		else if (ch > UNI_MAX_UTF16) 
		{
			if (flags == ConversionStrict) 
			{
				result = ConversionResultSourceIllegal;
			} else 
			{
				*target++ = UNI_REPLACEMENT_CHAR;
			}
		} 
		else 
		{
			/* target is a character in range 0xFFFF - 0x10FFFF. */
			if (target + 1 >= targetEnd) 
			{
				--source; /* Back up source pointer! */
				result = ConversionResultTargetExhausted; break;
			}
			ch -= halfBase;
			*target++ = (ch >> halfShift) + UNI_SUR_HIGH_START;
			*target++ = (ch & halfMask) + UNI_SUR_LOW_START;
		}
	}
	*sourceStart = source;
	*targetStart = target;
	return result;
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF16toUTF32 (
		const UTF16** sourceStart, const UTF16* sourceEnd, 
		UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags) 
{
	ConversionResult result = ConversionOk;
	const UTF16* source = *sourceStart;
	UTF32* target = *targetStart;
	UTF32 ch, ch2;
	while (source < sourceEnd) 
	{
		const UTF16* oldSource = source; /*  In case we have to back up because of target overflow. */
		ch = *source++;
		if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END && source < sourceEnd) 
		{
			ch2 = *source;
			if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
			{
				ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
					+ (ch2 - UNI_SUR_LOW_START) + halfBase;
				++source;
			} 
			else if (flags == ConversionStrict) 
			{ 
				/* it's an unpaired high surrogate */
				--source; /* return to the illegal value itself */
				result = ConversionResultSourceIllegal;
				break;
			}
		} 
		else if ((flags == ConversionStrict) && (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)) 
		{
			/* an unpaired low surrogate */
			--source; /* return to the illegal value itself */
			result = ConversionResultSourceIllegal;
			break;
		}
		if (target >= targetEnd) 
		{
			source = oldSource; /* Back up source pointer! */
			result = ConversionResultTargetExhausted; break;
		}
		*target++ = ch;
	}
	*sourceStart = source;
	*targetStart = target;
#ifdef CVTUTF_DEBUG
if (result == ConversionResultSourceIllegal) {
		fprintf(stderr, "ConvertUTF16toUTF32 illegal seq 0x%04x,%04x\n", ch, ch2);
		fflush(stderr);
}
#endif
	return result;
}

/* --------------------------------------------------------------------- */

/*
* Index into the table below with the first byte of a UTF-8 sequence to
* get the number of trailing bytes that are supposed to follow it.
*/
static const char trailingBytesForUTF8[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
* Magic values subtracted from a buffer value during UTF8 conversion.
* This table contains as many values as there might be trailing bytes
* in a UTF-8 sequence.
*/
static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
					0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
* Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
* into the first byte, depending on how many bytes follow.  There are
* as many entries in this table as there are UTF-8 sequence types.
* (I.e., one byte sequence, two byte... six byte sequence.)
*/
static const UTF8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

/* --------------------------------------------------------------------- */

/* The interface converts a whole buffer to avoid function-call overhead.
* Constants have been gathered. Loops & conditionals have been removed as
* much as possible for efficiency, in favor of drop-through switches.
* (See "Note A" at the bottom of the file for equivalent code.)
* If your compiler supports it, the "IsLegalUTF8" call can be turned
* into an inline function.
*/

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF16toUTF8 (
		const UTF16** sourceStart, const UTF16* sourceEnd, 
		UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags) 
{
	ConversionResult result = ConversionOk;
	const UTF16* source = *sourceStart;
	UTF8* target = *targetStart;
	while (source < sourceEnd) 
	{
		UTF32 ch;
		unsigned short bytesToWrite = 0;
		const UTF32 byteMask = 0xBF;
		const UTF32 byteMark = 0x80; 
		const UTF16* oldSource = source; /* In case we have to back up because of target overflow. */
		ch = *source++;
		/* If we have a surrogate pair, convert to UTF32 first. */
		if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END && source < sourceEnd) 
		{
			UTF32 ch2 = *source;
			if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
			{
				ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
					+ (ch2 - UNI_SUR_LOW_START) + halfBase;
				++source;
			} 
			else if (flags == ConversionStrict) 
			{ 
				/* it's an unpaired high surrogate */
				--source; /* return to the illegal value itself */
				result = ConversionResultSourceIllegal;
				break;
			}
		} 
		else if ((flags == ConversionStrict) && (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)) 
		{
			--source; /* return to the illegal value itself */
			result = ConversionResultSourceIllegal;
			break;
		}
		/* Figure out how many bytes the result will require */
		if (ch < (UTF32)0x80)				{ bytesToWrite = 1; } 
		else if (ch < (UTF32)0x800)			{ bytesToWrite = 2; } 
		else if (ch < (UTF32)0x10000)		{	bytesToWrite = 3; } 
		else if (ch < (UTF32)0x200000)		{ bytesToWrite = 4; } 
		else								{ bytesToWrite = 2; ch = UNI_REPLACEMENT_CHAR; }

		target += bytesToWrite;
		if (target > targetEnd) 
		{
			source = oldSource; /* Back up source pointer! */
			target -= bytesToWrite; result = ConversionResultTargetExhausted; break;
		}
		switch (bytesToWrite) 
		{
			/* note: everything falls through. */
			case 4:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 3:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 2:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 1:	*--target =  ch | firstByteMark[bytesToWrite];
		}
		target += bytesToWrite;
	}
	*sourceStart = source;
	*targetStart = target;
	return result;
}

/* --------------------------------------------------------------------- */

/*
* Utility routine to tell whether a sequence of bytes is legal UTF-8.
* This must be called with the length pre-determined by the first byte.
* If not calling this from ConvertUTF8to*, then the length can be set by:
*	length = trailingBytesForUTF8[*source]+1;
* and the sequence is illegal right away if there aren't that many bytes
* available.
* If presented with a length > 4, this returns false.  The Unicode
* definition of UTF-8 goes up to 4-byte sequences.
*/

static bool IsLegalUTF8(const UTF8 *source, int length) 
{
	UTF8 a;
	const UTF8 *srcptr = source+length;
	switch (length) 
	{
	default: return false;
	/* Everything else falls through when "true"... */
	case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
	case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
	case 2: if ((a = (*--srcptr)) > 0xBF) return false;
		switch (*source) 
		{
				/* no fall-through in this inner switch */
				case 0xE0: if (a < 0xA0) return false; break;
				case 0xF0: if (a < 0x90) return false; break;
				case 0xF4: if (a > 0x8F) return false; break;
				default:  if (a < 0x80) return false;
		}
	case 1: if (*source >= 0x80 && *source < 0xC2) return false;
	if (*source > 0xF4) return false;
	}
	return true;
}

/* --------------------------------------------------------------------- */

/*
* Exported function to return whether a UTF-8 sequence is legal or not.
* This is not used here; it's just exported.
*/
bool IsLegalUTF8Sequence(const UTF8 *source, const UTF8 *sourceEnd) 
{
	int length = trailingBytesForUTF8[*source]+1;
	if (source+length > sourceEnd) 
	{
			return false;
	}
	return IsLegalUTF8(source, length);
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF8toUTF16 (
		const UTF8** sourceStart, const UTF8* sourceEnd, 
		UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags) 
{
	ConversionResult result = ConversionOk;
	const UTF8* source = *sourceStart;
	UTF16* target = *targetStart;
	while (source < sourceEnd) 
	{
		UTF32 ch = 0;
		unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
		if (source + extraBytesToRead >= sourceEnd) 
		{
			result = ConversionResultSourceExhausted; break;
		}
		/* Do this check whether lenient or strict */
		if (! IsLegalUTF8(source, extraBytesToRead+1)) 
		{
			result = ConversionResultSourceIllegal;
			break;
		}
		/*
		* The cases all fall through. See "Note A" below.
		*/
		switch (extraBytesToRead) 
		{
			case 3:	ch += *source++; ch <<= 6;
			case 2:	ch += *source++; ch <<= 6;
			case 1:	ch += *source++; ch <<= 6;
			case 0:	ch += *source++;
		}
		ch -= offsetsFromUTF8[extraBytesToRead];

		if (target >= targetEnd) 
		{
			source -= (extraBytesToRead+1);	/* Back up source pointer! */
			result = ConversionResultTargetExhausted; break;
		}
		if (ch <= UNI_MAX_BMP) 
		{ 
			/* Target is a character <= 0xFFFF */
			if ((flags == ConversionStrict) && (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)) 
			{
				source -= (extraBytesToRead+1); /* return to the illegal value itself */
				result = ConversionResultSourceIllegal;
				break;
			} 
			else 
			{
				*target++ = ch;	/* normal case */
			}
		} 
		else if (ch > UNI_MAX_UTF16) 
		{
			if (flags == ConversionStrict) 
			{
				result = ConversionResultSourceIllegal;
				source -= (extraBytesToRead+1); /* return to the start */
				break; /* Bail out; shouldn't continue */
			} 
			else 
			{
				*target++ = UNI_REPLACEMENT_CHAR;
			}
		} 
		else 
		{
			/* target is a character in range 0xFFFF - 0x10FFFF. */
			if (target + 1 >= targetEnd) 
			{
				source -= (extraBytesToRead+1);	/* Back up source pointer! */
				result = ConversionResultTargetExhausted; break;
			}
			ch -= halfBase;
			*target++ = (ch >> halfShift) + UNI_SUR_HIGH_START;
			*target++ = (ch & halfMask) + UNI_SUR_LOW_START;
		}
	}
	*sourceStart = source;
	*targetStart = target;
	return result;
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF32toUTF8 (
		const UTF32** sourceStart, const UTF32* sourceEnd, 
		UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags) 
{
	ConversionResult result = ConversionOk;
	const UTF32* source = *sourceStart;
	UTF8* target = *targetStart;
	while (source < sourceEnd) 
	{
		UTF32 ch;
		unsigned short bytesToWrite = 0;
		const UTF32 byteMask = 0xBF;
		const UTF32 byteMark = 0x80; 
		ch = *source++;
		/* surrogates of any stripe are not legal UTF32 characters */
		if (flags == ConversionStrict ) 
		{
			if ((ch >= UNI_SUR_HIGH_START) && (ch <= UNI_SUR_LOW_END)) 
			{
				--source; /* return to the illegal value itself */
				result = ConversionResultSourceIllegal;
				break;
			}
		}
		/* Figure out how many bytes the result will require */
		if (ch < (UTF32)0x80)				{ bytesToWrite = 1; } 
		else if (ch < (UTF32)0x800)			{ bytesToWrite = 2; }
		else if (ch < (UTF32)0x10000)		{ bytesToWrite = 3; }
		else if (ch < (UTF32)0x200000)		{ bytesToWrite = 4; }
		else								{ bytesToWrite = 2; ch = UNI_REPLACEMENT_CHAR; }
		
		target += bytesToWrite;
		if (target > targetEnd) 
		{
			--source; /* Back up source pointer! */
			target -= bytesToWrite; result = ConversionResultTargetExhausted; break;
		}
		switch (bytesToWrite) 
		{
			/* note: everything falls through. */
			case 4:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 3:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 2:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 1:	*--target =  ch | firstByteMark[bytesToWrite];
		}
		target += bytesToWrite;
	}
	*sourceStart = source;
	*targetStart = target;
	return result;
}

/* --------------------------------------------------------------------- */

ConversionResult ConvertUTF8toUTF32 (
		const UTF8** sourceStart, const UTF8* sourceEnd, 
		UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags) 
{
	ConversionResult result = ConversionOk;
	const UTF8* source = *sourceStart;
	UTF32* target = *targetStart;
	while (source < sourceEnd) 
	{
		UTF32 ch = 0;
		unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
		if (source + extraBytesToRead >= sourceEnd) 
		{
			result = ConversionResultSourceExhausted; break;
		}
		/* Do this check whether lenient or strict */
		if (! IsLegalUTF8(source, extraBytesToRead+1)) 
		{
			result = ConversionResultSourceIllegal;
			break;
		}
		/*
		* The cases all fall through. See "Note A" below.
		*/
		switch (extraBytesToRead) 
		{
			case 3:	ch += *source++; ch <<= 6;
			case 2:	ch += *source++; ch <<= 6;
			case 1:	ch += *source++; ch <<= 6;
			case 0:	ch += *source++;
		}
		ch -= offsetsFromUTF8[extraBytesToRead];

		if (target >= targetEnd) 
		{
			source -= (extraBytesToRead+1);	/* Back up the source pointer! */
			result = ConversionResultTargetExhausted; break;
		}
		if (ch <= UNI_MAX_UTF32) 
		{
			*target++ = ch;
		} 
		else 
		{ 
			/* i.e., ch > UNI_MAX_UTF32 */
			*target++ = UNI_REPLACEMENT_CHAR;
		}
	}
	*sourceStart = source;
	*targetStart = target;
	return result;
}

/* ---------------------------------------------------------------------

	Note A.
	The fall-through switches in UTF-8 reading code save a
	temp variable, some decrements & conditionals.  The switches
	are equivalent to the following loop:
		{
			int tmpBytesToRead = extraBytesToRead+1;
			do {
				ch += *source++;
				--tmpBytesToRead;
				if (tmpBytesToRead) ch <<= 6;
			} while (tmpBytesToRead > 0);
		}
	In UTF-8 writing code, the switches on "bytesToWrite" are
	similarly unrolled loops.

	--------------------------------------------------------------------- */

}

using namespace Unicode;

XStringA ConvertHighAsciiToUTF8( const XStringA& src )
{
	// check first if string needs no conversion
	int len = src.Length();

	bool needsUtf = false;
	for ( int i = 0; i < len; i++ )
	{
		if ( (unsigned char) src[i] > 127 ) { needsUtf = true; break; }
	}

	if ( !needsUtf ) return src;

	// convert to 16 bit then down to utf8.
	UTF16* tempSrc = (UTF16*) malloc( (len + 1) * 2 );
	if ( tempSrc == NULL ) { ASSERT(false); return ""; }

	UTF8* tempDst = (UTF8*) malloc( (len + 1) * 2 );
	if ( tempDst == NULL ) { ASSERT(false); free(tempSrc); return ""; }

	for ( int i = 0; i < len; i++ )
		tempSrc[i] = (unsigned char) src[i];

	tempSrc[len] = 0;

	const UTF16* srcPos = tempSrc;
	UTF8* dstPos = tempDst;

	ConversionResult res = ConvertUTF16toUTF8( &srcPos, tempSrc + len + 1, &dstPos, tempDst + (len + 1) * 2, ConversionStrict );  
	ASSERT( res == ConversionOk );

	int dstLen = dstPos - tempDst;

	XStringA sres( (char*) tempDst );

	free( tempSrc );
	free( tempDst );

	return sres;
}

XStringA ConvertUTF8ToHighAscii( const XStringA& src )
{
	// check first if string needs no conversion
	int len = src.Length();

	bool needsUtf = false;
	for ( int i = 0; i < len; i++ )
	{
		if ( (unsigned char) src[i] > 127 ) { needsUtf = true; break; }
	}

	if ( !needsUtf ) return src;

	const UTF8* srcPos = (const UTF8*) ( (LPCSTR) src );
	const UTF8* srcEnd = srcPos + len + 1;

	UTF16* tempDst = (UTF16*) malloc( 2 * (len + 1) );
	if ( tempDst == NULL ) { ASSERT( false); return ""; }

	UTF16* dstPos = tempDst;
	UTF16* dstEnd = tempDst + len;

	ConversionResult res = ConvertUTF8toUTF16( &srcPos, srcEnd, &dstPos, dstEnd, ConversionStrict );
	ASSERT( res == ConversionOk );

	int dstLen = dstPos - tempDst - 1;

	XStringA sres;
	char* buff = sres.GetBuffer( dstLen + 1 );
	for ( int i = 0; i < dstLen; i++ )
		buff[i] = (char) tempDst[i];

	buff[dstLen] = 0;

	sres.ReleaseBuffer();

	free( tempDst );
	
	return sres;
}


XStringW ConvertUTF8ToUTF16( const XStringA& src )
{
	int len = src.Length();
	if ( len == 0 ) return L"";

	const UTF8* srcPos = (const UTF8*) ( (LPCSTR) src );
	const UTF8* srcEnd = srcPos + len + 1;

	XStringW sres;
	UTF16* tempDst = (UTF16*) sres.GetBuffer( 2 + len * 4 / 3 );

	//UTF16* tempDst = (UTF16*) malloc( 2 * (len + 2) );
	//if ( tempDst == NULL ) { ASSERT( false); return L""; }

	UTF16* dstPos = tempDst;
	UTF16* dstEnd = tempDst + len + 1;

	ConversionResult res = ConvertUTF8toUTF16( &srcPos, srcEnd, &dstPos, dstEnd, ConversionStrict );
	ASSERT( res == ConversionOk );

	int dstLen = dstPos - tempDst - 1;

	//XStringW sres;
	//wchar_t* buff = sres.GetBuffer( dstLen + 1 );
	//for ( int i = 0; i < dstLen; i++ )
	//	buff[i] = (wchar_t) tempDst[i];

	//buff[dstLen] = 0;
	tempDst[dstLen] = 0;

	sres.ReleaseBuffer();

	//free( tempDst );
	
	return sres;
}

XStringA ConvertUTF16ToUTF8( const XStringW& src )
{
	int len = src.Length();
	if ( len == 0 ) return "";

	const UTF16* srcPos = (const UTF16*) ( (LPCWSTR) src );
	const UTF16* srcEnd = srcPos + len + 1;

	XStringA sres;
	UTF8* tempDst = (UTF8*) sres.GetBuffer( 4 * (len + 2) );

	//UTF8* tempDst = (UTF8*) malloc( 4 * (len + 2) );
	//if ( tempDst == NULL ) { ASSERT( false); return ""; }

	UTF8* dstPos = tempDst;
	UTF8* dstEnd = tempDst + len * 4 + 1;

	ConversionResult res = ConvertUTF16toUTF8( &srcPos, srcEnd, &dstPos, dstEnd, ConversionStrict );
	ASSERT( res == ConversionOk );

	int dstLen = dstPos - tempDst - 1;

	//XStringA sres;
	//char* buff = sres.GetBuffer( dstLen + 1 );
	//for ( int i = 0; i < dstLen; i++ )
	//	buff[i] = (char) tempDst[i];

	tempDst[dstLen] = 0;

	sres.ReleaseBuffer();

	//free( tempDst );
	
	return sres;
}

bool ConvertUTF16ToUTF8( const wchar_t* src, size_t srcLen, char* dst, size_t& dstLen, bool relaxNullTerminator )
{
	if ( dst ) *dst = 0;

	if ( src == NULL )
	{
		if ( !relaxNullTerminator && dstLen == 0 ) return false;
		dstLen = 0;
		return true;
	}

	if ( srcLen == -1 ) srcLen = wcslen( src );

	const UTF16* srcPos = (const UTF16*) src;
	const UTF16* srcEnd = srcPos + srcLen;

	UTF8* dstPos = (UTF8*) dst;
	UTF8* dstEnd = (UTF8*) (dst + dstLen);

	ConversionResult res = ConvertUTF16toUTF8( &srcPos, srcEnd, &dstPos, dstEnd, ConversionStrict );
	if ( res == ConversionResultTargetExhausted )
		return false;
	else
		ASSERT( res == ConversionOk );

	size_t actualDstLen = dstPos - (UTF8*) dst;

	if ( dstLen > actualDstLen )
		dst[actualDstLen] = 0;
	else if ( !relaxNullTerminator )
		return false;

	dstLen = actualDstLen;

	return true;
}

bool ConvertUTF8ToUTF16( const char* src, size_t srcLen, wchar_t* dst, size_t& dstLen, bool relaxNullTerminator )
{
	if ( dst ) *dst = 0;

	if ( src == NULL )
	{
		if ( !relaxNullTerminator && dstLen == 0 ) return false;
		dstLen = 0;
		return true;
	}

	if ( srcLen == -1 ) srcLen = strlen( src );

	const UTF8* srcPos = (const UTF8*) src;
	const UTF8* srcEnd = srcPos + srcLen;

	UTF16* dstPos = (UTF16*) dst;
	UTF16* dstEnd = (UTF16*) (dst + dstLen);

	ConversionResult res = ConvertUTF8toUTF16( &srcPos, srcEnd, &dstPos, dstEnd, ConversionStrict );
	if ( res == ConversionResultTargetExhausted )
		return false;
	else
		ASSERT( res == ConversionOk );

	size_t actualDstLen = dstPos - (UTF16*) dst;

	if ( dstLen > actualDstLen )
		dst[actualDstLen] = 0;
	else if ( !relaxNullTerminator )
		return false;

	dstLen = actualDstLen;

	return true;
}


bool PAPI ConvertUTF16ToUTF8_Optimistic( const wchar_t* src, size_t srcLen, char* dst, size_t& dstLen, bool relaxNullTerminator )
{
	bool needProper = false;
	const wchar_t* s = src;
	char* d = dst;
	while ( *s && s - src < (int) srcLen )
	{
		if ( s - src == dstLen ) return false;
		if ( *s > 127 ) { needProper = true; break; }
		*d++ = *s++;
	}

	if ( !needProper )
	{
		if ( s - src == dstLen )
		{
			if ( !relaxNullTerminator ) return false;
		}
		else *d = 0;
		dstLen = s - src;
		return true;
	}

	return ConvertUTF16ToUTF8( src, srcLen, dst, dstLen, relaxNullTerminator );
}
