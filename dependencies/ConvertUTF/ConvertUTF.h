/*
 * Copyright 2001-2004 Unicode, Inc.
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

    Conversions between UTF32, UTF-16, and UTF-8.  Header file.

    Several funtions are included here, forming a complete set of
    conversions between the three formats.  UTF-7 is not included
    here, but is handled in a separate source file.

    Each of these routines takes pointers to input buffers and output
    buffers.  The input buffers are const.

    Each routine converts the text between *sourceStart and sourceEnd,
    putting the result into the buffer between *targetStart and
    targetEnd. Note: the end pointers are *after* the last item: e.g.
    *(sourceEnd - 1) is the last item.

    The return result indicates whether the conversion was successful,
    and if not, whether the problem was in the source or target buffers.
    (Only the first encountered problem is indicated.)

    After the conversion, *sourceStart and *targetStart are both
    updated to point to the end of last text successfully converted in
    the respective buffers.

    Input parameters:
	sourceStart - pointer to a pointer to the source buffer.
		The contents of this are modified on return so that
		it points at the next thing to be converted.
	targetStart - similarly, pointer to pointer to the target buffer.
	sourceEnd, targetEnd - respectively pointers to the ends of the
		two buffers, for overflow checking only.

    These conversion functions take a ConversionFlags argument. When this
    flag is set to strict, both irregular sequences and isolated surrogates
    will cause an error.  When the flag is set to lenient, both irregular
    sequences and isolated surrogates are converted.

    Whether the flag is strict or lenient, all illegal sequences will cause
    an error return. This includes sequences such as: <F4 90 80 80>, <C0 80>,
    or <A0> in UTF-8, and values above 0x10FFFF in UTF-32. Conformant code
    must check for illegal sequences.

    When the flag is set to lenient, characters over 0x10FFFF are converted
    to the replacement character; otherwise (when the flag is set to strict)
    they constitute an error.

    Output parameters:
	The value "sourceIllegal" is returned from some routines if the input
	sequence is malformed.  When "sourceIllegal" is returned, the source
	value will point to the illegal value that caused the problem. E.g.,
	in UTF-8 when a sequence is malformed, it points to the start of the
	malformed sequence.

    Author: Mark E. Davis, 1994.
    Rev History: Rick McGowan, fixes & updates May 2001.
		 Fixes & updates, Sept 2001.

------------------------------------------------------------------------ */

#ifndef CONVERT_UTF_INCLUDED
#define CONVERT_UTF_INCLUDED

#include <stdint.h>

namespace xo {
#ifdef _STRING_
std::wstring XO_API ConvertUTF8ToWide(const std::string& src);
std::string XO_API  ConvertWideToUTF8(const std::wstring& src);
inline std::wstring towide(const std::string& src) { return ConvertUTF8ToWide(src); }
inline std::string  toutf8(const std::wstring& src) { return ConvertWideToUTF8(src); }
#endif

/** Convert wchar_t (UTF16 or UTF32) to UTF8.
@param src Source buffer. May be NULL, which is equivalent to making srcLen = 0.
@param srcLen Length in characters of source. If -1, then we determine the length by looking for a null terminator.
@param dst The destination buffer.
@param dstLen The length in characters of the destination buffer.
@param relaxNullTerminator If true, then we don't make sure that we can add a null terminator to dst.
**/
bool XO_API ConvertWideToUTF8(const wchar_t* src, size_t srcLen, char* dst, size_t& dstLen, bool relaxNullTerminator = false);

/// Analogue of ConvertWideToUTF8
bool XO_API ConvertUTF8ToWide(const char* src, size_t srcLen, wchar_t* dst, size_t& dstLen, bool relaxNullTerminator = false);

bool XO_API ConvertWideToUTF16(const wchar_t* src, size_t srcLen, uint16_t* dst, size_t& dstLen, bool relaxNullTerminator = false);
bool XO_API ConvertUTF16ToWide(const uint16_t* src, size_t srcLen, wchar_t* dst, size_t& dstLen, bool relaxNullTerminator = false);

/** Returns the maximum number of UTF8 bytes necessary in order to represent any legal UTF16 string of the indicated number of UTF16 characters.

Maximum length of a UTF8 sequence is 4 bytes.
Maximum length of a UTF16 sequence is 2 bytes.
Therefore, when converting a UTF16 string to UTF8, in order to be safe against any possible inputs, you must allocate
N * 4 bytes, where N is the number of 16-bit characters. However, 4 byte UTF8 sequences are only necessary for values
of 0x10000 and greater. Likewise, UTF16 pairs are only necessary for values greater than or equal to 0x10000. So,
for values less than 0x10000, we have the maximum number of UTF8 bytes equal to N * 3. For values greater than or
equal to 0x10000, we need (N/2) * 4 = N * 2. So the maximum number of bytes necessary is N * 3.

**/
inline size_t MaximumUtf8FromUtf16(size_t utf16Len) {
	return utf16Len * 3;
}

inline size_t MaximumUtf8FromUtf32(size_t utf32Len) {
	return utf32Len * 4;
}

inline size_t MaximumUtf8FromWide(size_t wideLen) {
	return sizeof(wchar_t) == 2 ? MaximumUtf8FromUtf16(wideLen) : MaximumUtf8FromUtf32(wideLen);
}

inline size_t MaximumUtf16FromUtf8(size_t utf8Len) {
	return utf8Len * 2;
}

inline size_t MaximumUtf32FromUtf8(size_t utf8Len) {
	return utf8Len;
}

inline size_t MaximumUtf32FromUtf16(size_t utf16Len) {
	return utf16Len;
}

inline size_t MaximumWideFromUtf8(size_t utf8Len) {
	return sizeof(wchar_t) == 2 ? MaximumUtf16FromUtf8(utf8Len) : MaximumUtf32FromUtf8(utf8Len);
}
}

namespace xo {
namespace Unicode {

/* ---------------------------------------------------------------------
    The following 4 definitions are compiler-specific.
    The C standard does not guarantee that wchar_t has at least
    16 bits, so wchar_t is no less portable than unsigned short!
    All should be unsigned values to avoid sign extension during
    bit mask & shift operations.
------------------------------------------------------------------------ */

typedef uint32_t UTF32; /* at least 32 bits */
typedef uint16_t UTF16; /* at least 16 bits */
typedef uint8_t  UTF8;  /* typically 8 bits */

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32) 0x0000FFFD
#define UNI_MAX_BMP (UTF32) 0x0000FFFF
#define UNI_MAX_UTF16 (UTF32) 0x0010FFFF
#define UNI_MAX_UTF32 (UTF32) 0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32) 0x0010FFFF

enum ConversionResult {
	ConversionOk,                    // Conversion successful.
	ConversionResultSourceExhausted, // Partial character in source, but hit end.
	ConversionResultTargetExhausted, // Insufficient room in target for conversion.
	ConversionResultSourceIllegal    // Source sequence is illegal/malformed.
};

enum ConversionFlags {
	ConversionStrict = 0,
	ConversionLenient
};

ConversionResult XO_API ConvertUTF8toUTF16(const UTF8** sourceStart, const UTF8* sourceEnd,
                                           UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags);

ConversionResult XO_API ConvertUTF16toUTF8(const UTF16** sourceStart, const UTF16* sourceEnd,
                                           UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags);

ConversionResult XO_API ConvertUTF8toUTF32(const UTF8** sourceStart, const UTF8* sourceEnd,
                                           UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags);

ConversionResult XO_API ConvertUTF32toUTF8(const UTF32** sourceStart, const UTF32* sourceEnd,
                                           UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags);

ConversionResult XO_API ConvertUTF16toUTF32(const UTF16** sourceStart, const UTF16* sourceEnd,
                                            UTF32** targetStart, UTF32* targetEnd, ConversionFlags flags);

ConversionResult XO_API ConvertUTF32toUTF16(const UTF32** sourceStart, const UTF32* sourceEnd,
                                            UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags);

bool XO_API IsLegalUTF8Sequence(const UTF8* source, const UTF8* sourceEnd);

} // namespace Unicode
} // namespace xo

#endif
