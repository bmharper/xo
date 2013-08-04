#pragma once

// String compatibility functions for porting from Windows to Linux

// The functions that take u16 pointers were written to deal with
// on-disk formats that are 16 bit little endian UCS-2 encoded strings.
// When originally writing that serialization code on Windows, I didn't
// stop to think that wchar_t was a terrible serialization type.

// The endianness is NOT respected by these functions - I'm just assuming for now that
// the entire planet is Little Endian, which seems reasonable enough, given all
// Android devices I've seen are LE, iOS is LE, Raspberry Pi is LE, etc.

// BMH 2013-03-15

PAPI int strcmp_u16_wchar		( const uint16* a, const wchar_t* b, int max_chars = -1 );
PAPI int stricmp_u16_wchar		( const uint16* a, const wchar_t* b, int max_chars = -1 );
PAPI int strcmp_u16_u16			( const uint16* a, const uint16* b, int max_chars = -1 );
PAPI int stricmp_u16_u16		( const uint16* a, const uint16* b, int max_chars = -1 );

