#include "pch.h"
#include <stdio.h>
#include <string.h>
#include <wctype.h>
#include "../modp/src/modp_b64.h"
#include "../Platform/winheaders.h"

void test()
{
	using namespace AbCore;
	//RegExPatternT< char >( "Hello" );
}

void instantiate_strings_cpp()
{
	XStringW a, b, c;
	c = a + b;

	XStringA d, e, f;
	d = e + f;
	d = e + 'a';
}


int MyStrLen( char* buff )
{
	int p = 0;
	while ( buff[p] != 0 ) p++;
	return p;
}

void GenerateDecimals( double v, int digits, char* low, int& lp, int& lp_nonzero, int& lp_little )
{
	double av = fabs(v);

	lp = 0;
	lp_nonzero = 0;		// counts the number of trailing zeros
	lp_little = 0;		// counts the number of leading zeros

	// generate a correctly-sorted 'low'.
	for ( double q = 10 * (av - floor(av)); q != 0; q = 10 * (q - floor(q)) )
	{
		int x = (int) floor(q);
		low[lp++] = x + '0';
		if ( x == 0 && lp_little == lp - 1 ) lp_little = lp;
		if ( x != 0 ) lp_nonzero = lp;
		if ( lp > digits ) break;
	}
	low[lp] = 0;

	// trim trailing zeros
	if ( true )
	{
		low[lp_nonzero] = 0;
		lp = lp_nonzero;
	}
}

// Trims trailing zeros after the decimal point
// preserveDotZero - If true, then leave 123.000 as 12.0
void TrimTrailingZeros( char* buff, bool preserveDotZero )
{
	int nonZero = -1;
	int decPoint = -1;
	for ( int p = 0; buff[p] != 0; p++ )
	{
		if ( buff[p] == '.' )
			decPoint = p;
		if ( buff[p] != '0' )
			nonZero = p;
	}
	if ( decPoint != -1 )
	{
		// 1.0300	-> 1.03
		// 1.000	-> 1.0		(preserveDotZero = true)
		// 1.000	-> 1.		(preserveDotZero = false)
		if ( preserveDotZero && decPoint == nonZero && buff[decPoint + 1] != 0 )
			buff[nonZero + 2] = 0;
		else
			buff[nonZero + 1] = 0;
	}
}

// Trims the trailing dot off a string
void TrimTrailingDot( char* buff )
{
	int len = MyStrLen( buff );
	if ( len > 0 )
	{
		if ( buff[len - 1] == '.' )
			buff[len - 1] = 0;
	}
}

// prepends a zero to a string starting with a period, if the length of that
// string is less than 'digits'
void PrependZeroIfSpace( char* buff, int digits )
{
	if ( buff[0] == 0 ) return;
	bool negative = buff[0] == '-';
	if (	buff[0] == '.' ||
				(negative && buff[1] == '.') )
	{
		int len = MyStrLen( buff );
		if ( len < digits )
		{
			int bottom = negative ? 1 : 0;
			for ( int i = len + 1; i > bottom; i-- )
				buff[i] = buff[i - 1];
			if ( negative )
			{
				buff[0] = '-';
				buff[1] = '0';
			}
			else
			{
				buff[0] = '0';
			}
		}
	}
}

#define ROUND_D( a, power ) ( floor(0.5 + (a) / power) * power )


void FormatFInternal( double v, char* buff, int digits, int& p )
{
	double av = fabs(v);
	double ex = log10( av );

	p = 0;
	bool isinteger = floor(av) == av;
	int before = ceil(ex);
	if ( before < 0 ) before = 0;
	int dot = 1;
	int neg = v < 0 ? 1 : 0;
	int rem = digits - before - dot - neg;
	rem = max(rem, 0);

	ASSERT( digits < 100 );
	char tbuff[200];
	char fbuff[20] = "%.xf";

	_itoa( rem, fbuff + 2, 10 );
	int flen = MyStrLen( fbuff );
	fbuff[flen++] = 'f';
	fbuff[flen] = 0;

	sprintf( tbuff, fbuff, v );
	tbuff[arraysize(tbuff) - 1] = 0;

	// remove all prepending zeros. they get added later if there's enough space
	int b = neg ? 1 : 0;
	if ( tbuff[b] == '0' && tbuff[b + 1] == '.' )
	{
		int i;
		for ( i = b; tbuff[i] != 0; i++ )
			tbuff[i] = tbuff[i + 1];
	}

	/*
	// remove a forward zero if there is not enough space. -0.10 ==> -.1
	if ( digits == 3 && neg && !isinteger )
	{
		tbuff[1] = tbuff[2];
		tbuff[2] = tbuff[3];
		tbuff[3] = 0;
	}
	*/

	int i;
	for ( i = 0; tbuff[i] != 0 && i < digits; i++ )
	{
		buff[i] = tbuff[i];
	}

	buff[i] = 0;
	p = i;
}

// Formats in the style -x.y
// Formatting will terminate without notice if there are not enough digits to represent
// the value, and the returned value will be generated up to the requested number of digits.
void FormatF( double v, char* buff, int digits, bool leaveFirstTrailingZero = false )
{
	int significant_digits = digits - ( leaveFirstTrailingZero ? 1 : 0 );
	int p;
	FormatFInternal( v, buff, significant_digits, p );
	buff[p] = 0;
	
	TrimTrailingZeros( buff, leaveFirstTrailingZero );
	if ( !leaveFirstTrailingZero )
		TrimTrailingDot( buff );
	
	PrependZeroIfSpace( buff, significant_digits );
}

// Formats in the style x[.y]eP
bool FormatG( double v, char* buff, int buff_size, int digits )
{
	int exi;
	if ( fabs(v) > 1 )	exi = floor( log10( fabs(v) ) + 0.5 );
	else				exi = floor( log10( fabs(v) ) + 0.5 );
	int exiA = abs(exi);
	int adig = digits;
	adig--;						// minus 1 for the 'e'
	
	int eDig = 0;
	if ( exi < 0 ) eDig++;		// plus 1 for the '-' before the 'e'
	if ( exiA <= 9 ) eDig += 1;
	else if ( exiA <= 99 ) eDig += 2;
	else if ( exiA <= 999 ) eDig += 3;
	else ASSERT(false); // double can represent up to e+308.
	adig -= eDig;

	if ( adig < 1 ) return false;
	if ( v < 0 && adig < 2 ) return false;

	double pv = v * pow( 10.0, -exi );
	FormatF( pv, buff, adig );
	int p = 0;
	for ( ; p < adig; p++ )
	{
		if ( buff[p] == 0 ) break;
	}

	// turn 'x.' into 'x'
	if ( buff[1] == '.' && buff[2] == 0 )
	{
		buff[1] = 0;
		p--;
	}

	buff[p++] = 'e';

	_itoa( exi, buff + p, 10 );
	
	ASSERT( p + eDig <= digits );
	buff[p + eDig] = 0;
	buff[digits] = 0; // paranoid
	
	return true;
}

void GenerateDecimals( double v, int digits, char* low )
{
	int lp, lp_nonzero, lp_little;
	GenerateDecimals( v, digits, low, lp, lp_nonzero, lp_little );
}

bool IsPreciseAt( double v, int po )
{
	double x = v * pow(10.0, -po);
	return floor(x) == x;
}

char FirstDigitBeforePoint( double v ) 
{
	if ( v < 0 )	return '0' + CLAMP( -int(v), 0, 9 );
	else			return '0' + CLAMP( int(v), 0, 9 );
}

char FirstDigitBeforePointRounded( double v ) 
{
	if ( v < 0 )	return '0' + CLAMP( -int(v + 0.5), 0, 9 );
	else			return '0' + CLAMP( int(v + 0.5), 0, 9 );
}

char FirstDigitAfterPointRounded( double v ) 
{
	if ( v < 0 )	return '0' + CLAMP( -int(v * 10 + 0.5), 0, 9 );
	else			return '0' + CLAMP( int(v * 10 + 0.5), 0, 9 );
}

template< typename TStr >
TStr TFloatToXString_New( double v, int digits, bool preferStraight, bool forceDecimalPoint = false )
{
	if ( digits < 1 ) { ASSERT(false); return ""; }
	if ( AbCore::IsNaN(v) )			return digits < 4 ? TStr("#NAN").Left( digits ) : "#NAN";
	if ( !AbCore::IsFinite(v) )		return digits < 4 ? TStr("#INF").Left( digits ) : "#INF";
	if ( !forceDecimalPoint )
	{
		if ( v == 1 )					return "1";
		if ( v == -1 && digits >= 2 )	return "-1";
		if ( v == 0 )					return "0";
	}
	else if ( digits >= 3 && forceDecimalPoint )
	{
		if ( v == 1 )					return "1.0";
		if ( v == -1 && digits >= 4 )	return "-1.0";
		if ( v == 0 )					return "0.0";
	}
	digits = CLAMP( digits, 1, 120 );

	bool integer = floor(v) == v;
	bool negative = v < 0;
	double av = fabs(v);
	int iv = floor(v + 0.5);

	char sint[128] = "";
	const char* unable = "#";

	_itoa( iv, sint, 10 );
	int sintLen = MyStrLen( sint );

	if ( digits == 1 )
	{
		if ( v >= 0 && v < 9.5 ) return TStr( FirstDigitBeforePointRounded(v) );
		return unable;
	}
	else if ( digits == 2 )
	{
		if ( v < -9.5 || v > 99.5 ) return unable;
		char b[3] = {0,0,0};
		if ( ( forceDecimalPoint || !integer ) && v > -0.5 && v < 9.5 )
		{
			FormatF( v, b, 2, forceDecimalPoint );
			return b;
		}
		else
		{
			ASSERT(sintLen <= 2);
			return sint;
		}
	}
	else if ( digits == 3 )
	{
		if ( v < -99 || v > 9e9 ) return unable;
		char b[4] = {0,0,0,0};
		if ( forceDecimalPoint || !integer )
		{
			FormatF( v, b, 3, forceDecimalPoint );
		}
		else
		{
			if ( v > 999 )
			{
				VERIFY( FormatG( v, b, sizeof(b) / sizeof(char), 3 ) );
			}
			else
			{
				ASSERT( sintLen <= 3);
				return sint;
			}
		}
		return b;
	}
	else if ( digits == 4 )
	{
		if ( v < -9e9 || v > 9e99 ) return unable;
		char b[5] = {0,0,0,0,0};
		if ( v <= -9.9 || v > 9999 )
		{
			VERIFY( FormatG( v, b, sizeof(b) / sizeof(char), 4 ) );
		}
		else
		{
			FormatF( v, b, 4, forceDecimalPoint );
		}
		return b;
	}
	else if ( digits == 5 )
	{
		if ( v < -9e99 ) return unable;
		char b[6] = {0,0,0,0,0,0};
		int bsize = sizeof(b) / sizeof(char);
		if ( v < -9999 )
		{
			VERIFY( FormatG( v, b, bsize, 5 ) );
		}
		else
		{
			if ( av < 0.001 )
			{
				int sd = negative ? 4 : 5;
				if ( IsPreciseAt( v, -(sd - 1) ) && preferStraight )
					FormatF( v, b, 5, forceDecimalPoint );
				else
					VERIFY( FormatG( v, b, bsize, 5 ) );
			}
			else
			{
				if ( av > 99999 )
					VERIFY( FormatG( v, b, bsize, 5 ) );
				else
					FormatF( v, b, 5, forceDecimalPoint );
			}
		}
		return b;
	}
	else
	{
		// 6 digits or more
		char buff[128] = "";
		int bsize = sizeof(buff) / sizeof(char);
		int prec = negative ? digits - 2 : digits - 1;
		int adig = digits;
		if ( negative ) adig--;
		double smax = pow(10.0, adig - 1);
		double smin = pow(10.0, -adig + 1);
		if ( av < smin || av >= smax )
		{
			// need to use scientific notation to represent the magnitude of the number
			VERIFY( FormatG( v, buff, bsize, digits ) );
		}
		else
		{
			// evaluate whether its better, equal, or worse to use scientific notation.
			if ( av < 0.001 )
			{
				// if the user prefers it, and he isn't losing precision, then grant him straight notation for
				// small numbers.
				if ( IsPreciseAt(v, -prec) && preferStraight )
					FormatF( v, buff, digits, forceDecimalPoint );
				else
					FormatG( v, buff, bsize, digits );
			}
			else
			{
				FormatF( v, buff, digits, forceDecimalPoint );
			}
		}
		return buff;
	}
}

XStringA FloatToXStringA( double v, int digits, bool preferStraight, bool forceDecimalPoint )
{ 
	return TFloatToXString_New<XStringA> ( v, digits, preferStraight, forceDecimalPoint );
}

XStringW FloatToXStringW( double v, int digits, bool preferStraight, bool forceDecimalPoint ) 
{ 
	return TFloatToXString_New<XStringW> ( v, digits, preferStraight, forceDecimalPoint );
}


XStringA IntToXStringA( INT64 v )
{
	char buff[128] = "";
	_i64toa( v, buff, 10 );
	return buff;
}

XStringW IntToXStringW( INT64 v )
{
	wchar_t buff[128] = L"";
	_i64tow( v, buff, 10 );
	return buff;
}


XStringW PAPI FloatToXStringWWithCommas( double v, int digits )
{
	return AbCore::AddCommas( FloatToXStringW( v, digits, true ) );
}

XStringA PAPI FloatToXStringAWithCommas( double v, int digits )
{
	return AbCore::AddCommas( FloatToXStringA( v, digits, true ) );
}

XStringA PAPI IntToXStringAWithCommas( int64 v )
{
	return AbCore::AddCommas( IntToXStringA(v) );
}

XStringW PAPI IntToXStringWWithCommas( int64 v )
{
	return AbCore::AddCommas( IntToXStringW(v) );
}

#ifndef _WIN32
#define _CVTBUFSIZE (309+40) /* # of digits in max. dp value + slop */
#endif

template< typename TCH >
bool TFloatToStringDB( TCH* buf, int digits, double v )
{
	char tbuf[_CVTBUFSIZE];
	int ptDec, ptSign;
	double av = fabs(v);
#ifdef _WIN32
	_ecvt_s( tbuf, v, digits, &ptDec, &ptSign );
#else
	ecvt_r( v, digits, &ptDec, &ptSign, tbuf, arraysize(tbuf) );
#endif

	if ( digits == 1 )
	{
		if ( v < 0 || v > 9 ) { buf[0] = '0'; buf[1] = 0; return false; }
		buf[0] = '0' + floor(v);
		buf[1] = 0;
		return true;
	}
	else if ( digits == 2 )
	{
		if ( v < -9 || v > 99 ) { buf[0] = '0'; buf[1] = 0; return false; }
		if ( v > 0 && v < 1 )
		{
			buf[0] = '.';
			buf[1] = (v * 10) + '0';
			buf[2] = 0;
		}
		else
		{
			_itoa( (int) v, tbuf, 10 );
			buf[0] = tbuf[0];
			buf[1] = tbuf[1];
			buf[2] = 0;
		}
		return true;
	}

	if ( !AbCore::IsFinite(v) || AbCore::IsNaN(v) )
	{
		const char* tinf = "#INF";
		const char* tnan = "#NAN";
		const char* txt = AbCore::IsNaN(v) ? tnan : tinf;
		for ( int i = 0; i < digits && txt[i]; i++ ) buf[i] = txt[i];
		buf[digits] = 0;
		return false;
	}
	
	int need;
	if ( ptDec < 0 )
	{
		need = 2 + -ptDec;
	}
	else
	{
		need = 2 + ptDec;
	}
	if ( ptSign != 0 ) need++;

	if ( need > digits && av > 1.0 )
	{
		// For numbers greater than 1.0, the question we're trying to answer here is "Can the magnitude of the value be represented without exponential notation?".
		// If so, then we always do that.
		int intNeed = ptDec;
		if ( ptSign ) intNeed++;
		if ( intNeed == digits )
		{
			// Represent as an integer
			need = digits;
		}
	}

	if ( need <= digits )
	{
		// Straight write is ok
		int i = 0, j = 0;
		if ( ptSign ) buf[j++] = '-';
		if ( ptDec < 0 )
		{
			buf[j++] = '.';
			for ( ; ptDec < 0; ptDec++ )
				buf[j++] = '0';
			ptDec = -1000;
		}
		for ( ; j < digits && tbuf[i];  )
		{
			if ( i == ptDec )
				buf[j++] = '.';
			buf[j++] = tbuf[i++];
		}
		buf[j] = 0;
	}
	else
	{
		// We need to do scientific notation
		int needForE = 1; // for the 'e' or the 'e'
		int lpow = 0;
		if ( av < 1.0 )
		{
			if ( av >= 1e-9 ) lpow = 2;
			else if ( av >= 1e-99 ) lpow = 3;
			else lpow = 4;
		}
		else
		{
			if ( av <= 9e9 ) lpow = 1;
			else if ( av <= 9e99 ) lpow = 2;
			else lpow = 3;
		}
		//if ( av < 1 ) lpow++; // for the '-' after the 'e'
		needForE += lpow;

		// sig is everything to the left of 'e', including the decimal point (if any)
		int sig = digits - needForE;
		if ( ptSign ) sig--;

		if ( sig < 1 )
		{
			buf[0] = 0;
			return false;
		}

		int i = 0, j = 0;
		if ( ptSign ) buf[j++] = '-';
		buf[j++] = tbuf[i++];
		if ( sig > 1 )
			buf[j++] = '.';
		for ( int k = 2; k < sig; k++ )
			buf[j++] = tbuf[i++];
		buf[j++] = 'e';
		char lbuf[30];
		double log10_av = log10(av);
		log10_av = floor(log10(av));
		//if ( av < 1.0 ) log10_av = -ceil(-log10_av);
		//else				log10_av = floor(log10_av);
		_itoa( log10_av, lbuf, 10 );
		buf[j++] = lbuf[0];
		if ( lpow > 1 ) buf[j++] = lbuf[1];
		if ( lpow > 2 ) buf[j++] = lbuf[2];
		if ( lpow > 3 ) buf[j++] = lbuf[3];
		ASSERT( j == digits );
		buf[j] = 0;
	}

	return true;
}

bool PAPI FloatToStringDB( char* buf, int digits, double v )
{
	return TFloatToStringDB( buf, digits, v );
}

bool PAPI FloatToStringDB( wchar_t* buf, int digits, double v )
{
	return TFloatToStringDB( buf, digits, v );
}

XStringA GetExtension( const XStringA &fname )
{
	XStringA ext;
	int pos = fname.ReverseFind( '.' );
	if (pos >= 0) ext = fname.Right( fname.Length() - pos );
	return ext;
}

XStringW GetExtension( const XStringW &fname )
{
	XStringW ext;
	int pos = fname.ReverseFind( '.' );
	if (pos >= 0) ext = fname.Right( fname.Length() - pos );
	return ext;
}

struct stoi_a
{
	static INT64 stoi( const XStringA& str )	{	return _atoi64( str ); }
};

struct stoi_w
{
	static INT64 stoi( const XStringW& str )	{	return _wtoi64( str ); }
};

struct isdig_a
{
	static bool isdig( char c )				{	return isdigit( (unsigned char) c ) != 0; }
};

struct isdig_w
{
	static bool isdig( wchar_t c)			{	return iswdigit( c ) != 0; }
};

template< typename TSTR, typename f_isdig, typename f_stoi >
TSTR TIncrementName( const TSTR& name )
{
	int lastDigit = name.Length();
	bool overLimit = false;
	int digLen = 1;
	INT64 numVal = 0;
	const int i64limit = 18;
	for ( int i = name.Length() - 1; i >= -1; i-- )
	{
		if ( i < 0 || !f_isdig::isdig( name[i] ) )
		{
			//ASSERT( digLen <= 19 );
			digLen = name.Length() - i - 1;
			lastDigit = i + 1;
			/*
			if ( digLen <= i64limit )
			{
				numVal = f_stoi::stoi( name.Mid( i + 1 ) );
			}
			else
			{
				overLimit = true;
			}
			*/
			// I have come to realize that it's probably faster on average simply to do things straight on the characters
			// than to go to int and back again.
			overLimit = true;
			break;
		}
	}

	if ( overLimit )
	{
		// have to do this manually! I should really write a 128-bit integer thingy, but this is quicker for now
		TSTR next = name;
		bool carry = false;
		for ( int i = name.Length() - 1; i >= lastDigit; i-- )
		{
			if ( next[i] == '9' )
			{
				next[i] = '0';
				carry = true;
			}
			else
			{
				next[i]++;
				carry = false;
				break;
			}
		}
		if ( carry || lastDigit == name.Length() )
		{
			// need more space (I am hoping that in most use cases, we will end up here infrequently)
			next.Insert( lastDigit, '1' );
		}
		return next;
	}
	else
	{
		// THIS IS NOW NEVER REACHED!
		numVal++;
		double logVal = log10( (double) numVal );
		if ( floor(logVal) == logVal )
		{
			// digLen cannot be less than logVal. It can only be exactly equal to it, or greater.
			if ( digLen == logVal ) digLen++; 
		}
		if ( digLen == 0 ) digLen = 1;
		TSTR formatStr;
		formatStr.Format( TSTR("%%0%dI64d"), digLen );
		TSTR next = name.Left( lastDigit ) + TSTR::FromFormat( formatStr, numVal );
		return next;
	}
}

XStringA IncrementName( const XStringA& name )
{
	return TIncrementName< XStringA, isdig_a, stoi_a > ( name );
}

XStringW IncrementName( const XStringW& name )
{
	return TIncrementName< XStringW, isdig_w, stoi_w > ( name );
}

// Fixes the extension of a filename (for saving usually). If caseSense is
// true, then case sensitiving is respected. If forceExtension, then the filename
// will be forced to have the given extension, even if one is already provided.
XString FixExtension( XString fname, XString ext, bool caseSense,
											bool forceExtension ) 
{
	if ( ext.Length() == 0 )
	{
		// chop the extension
		int lastDotPos = fname.ReverseFind('.');
		int lastSlash = fname.ReverseFind( DIR_SEP );
		if ( lastDotPos > 0 && lastDotPos > lastSlash )
		{
			return fname.Left( lastDotPos );
		}
		// don't know...
		return fname;
	}

	XString& fn = fname;
	XString existingExt;

	// remove any dot from the specified extension
	if ( ext.Left(1) == '.' ) ext = ext.Mid( 1 );

	int lastDotPos = fn.ReverseFind('.');
	
	// respect a thing like "project1.mydrawing"
	if (lastDotPos != -1 && lastDotPos < fn.Length()-1) {
		existingExt = fn.Mid(lastDotPos+1);
	}
	// already got a dot at the end
	if (	lastDotPos != -1 && 
				!forceExtension && 
				abs(existingExt.Length() - ext.Length()) < 2 ) 
	{
		return fn;
	}
	// fix a badly cased extension
	if (	caseSense && existingExt.EqualsNoCase(ext) &&
				!existingExt.Equals(ext) ) 
	{
		fn = fn.Left(lastDotPos) + _T(".") + ext;
		return fn;
	}

	if ( !caseSense && existingExt.EqualsNoCase(ext) ) 
	{
		return fn;
	}

	// chop any trailing dots
	while ( lastDotPos == fn.Length()-1 && lastDotPos > 0 ) 
	{
		fn = fn.Left( fn.Length() - 1 );
		lastDotPos = fn.ReverseFind('.');
	}

	// user entered a bogus string full of dots.....
	if (lastDotPos == 0) 
	{
		fn = "Untitled";
	}

	// last resort. Add the extension onto the end
	fn = fn + _T(".") + ext;
	return fn;
}

/*
// encode 3 8-bit binary bytes as 4 '6-bit' characters
void EncodeBlockB64( const char* cb64, const unsigned char bin[3], char bout[4], int len )
{
	unsigned char bin1 = len >= 2 ? bin[1] : 0;
	unsigned char bin2 = len >= 3 ? bin[2] : 0;
	bout[0] = cb64[ bin[0] >> 2 ];
	bout[1] = cb64[ ((bin[0] & 0x03) << 4) | ((bin1 & 0xf0) >> 4) ];
	bout[2] = (char) (len > 1 ? cb64[ ((bin1 & 0x0f) << 2) | ((bin2 & 0xc0) >> 6) ] : '=');
	bout[3] = (char) (len > 2 ? cb64[ bin2 & 0x3f ] : '=');
}

XStringA EncodeBase64( int bytes, const void* src )
{
	podvec<char> res;
	const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char* bsrc = (unsigned char*) src;
	char bout[4];
	for ( int i = 0; i < bytes; i += 3 )
	{
		EncodeBlockB64( cb64, bsrc + i, bout, bytes - i );
		res += bout[0];
		res += bout[1];
		res += bout[2];
		res += bout[3];
	}
	res += 0;
	return &res[0];
}
*/

XStringA EncodeBase64( int bytes, const void* src )
{
	XStringA r;
	r.Resize( modp_b64_encode_len(bytes) - 1 );
	modp_b64_encode( r.GetRawBuffer(), (const char*) src, bytes );
	return r;
}

// Erg.. this version tolerates whitespace. I need to separate this out into two versions.
// The one can be fast (modp), and this one slow.
bool DecodeBase64( const char* src, int srclen, int& length, void* dst )
{
	/*
	length = modp_burl_decode( (char*) dst, src, srclen );
	return length != -1;
	*/
	int t = 0;
	length = 0;
	unsigned char* bdst = (unsigned char*) dst;
	if ( srclen == -1 ) srclen = (int) strlen(src);
	for ( int i = 0; i < srclen; )
	{
		// we nibble off 4 characters at a time
		char inp[4] = {'=', '=', '=', '='};
		int nb = 0;
		int ij = 0;
		while ( nb < 4 )
		{
			if ( i + ij >= srclen ) break;
			else
			{
				char ch = src[i + ij];
				// skip whitespace
				if ( ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ) ij++;
				else
				{
					// normal character
					inp[nb++] = ch;
					ij++;
				}
			}
		}
		unsigned int vt = 0;
		int len = 4;
		for ( int j = 0; j < 4; j++ )
		{
			char ch = inp[j];
			unsigned int v = 0;
			if		( ch >= 'A' && ch <= 'Z' )						v = ch - 'A' + 0;
			else if ( ch >= 'a' && ch <= 'z' )						v = ch - 'a' + 26;
			else if ( ch >= '0' && ch <= '9' )						v = ch - '0' + 52;
			else if ( ch == '+' )									v = 62;
			else if ( ch == '/' )									v = 63;
			else if ( ch == '=' )									{ v = 0; if ( len == 4 ) len = j; }
			else return false;
			vt <<= 6;
			vt |= v;
		}
		if ( len < 2 ) return false;
		bdst[t++] = (vt >> 16) & 0xFF;
		if ( len > 2 ) bdst[t++] = (vt >> 8) & 0xFF;
		if ( len > 3 ) bdst[t++] = (vt) & 0xFF;
		if ( len < 4 ) break;
		i += ij;
	}
	length = t;
	return true;
}

bool PAPI DecodeBase64( const XStringA& src, int& length, void* dst )
{
	return DecodeBase64( (const char*) src, src.Length(), length, dst );
}

podvec<byte> PAPI DecodeBase64( const XStringA& src )
{
	podvec<byte> buf;
	buf.reserve_uninitialized( DecodeBase64_DecodeLen(src.Length()) );
	int length = 0;
	if ( !DecodeBase64( src, length, buf.data ) )
		return podvec<byte>();
	buf.count = length;
	return buf;
}

std::string PAPI DecodeBase64_Fast( const XStringA& src )
{
	std::string res;
	res.resize( modp_b64_decode_len(src.Length()) );
	size_t reslen = modp_b64_decode( &res[0], src, src.Length() );
	if ( reslen == -1 )
	{
		res.resize(0);
	}
	else
	{
		res.resize(reslen);
	}
	return res;
}

void PAPI DecodeBase64_Fast( const XStringA& src, int& length, void* dst )
{
	length = (int) modp_b64_decode( (char*) dst, src, src.Length() );
}

PAPI bool ReplaceText_Tokens( const wchar_t* tOpen, const wchar_t* tClose, const XStringW& original, XStringW& final, XStringW& temp, void* context, ReplaceText_Callback callback )
{
	final.ClearNoAlloc();
	temp.ClearNoAlloc();
	
	int tLen = (int) wcslen(tOpen);
	if ( wcslen(tClose) != tLen ) { ASSERT(false); return false; }
	if ( tLen != 1 && tLen != 2 ) { ASSERT(false); return false; }
	bool tDouble = tLen == 2;

	int count = 0;
	wchar_t prev = 0;
	for ( int pos = 0; pos < original.Length(); pos++ )
	{
		final += original[pos];
		temp += original[pos];

		if ( (tDouble && prev == tOpen[0] && original[pos] == tOpen[1]) || (!tDouble && original[pos] == tOpen[0]) )
		{
			temp.ClearNoAlloc();
		}
		else if ( (tDouble && prev == tClose[0] && original[pos] == tClose[1]) || (!tDouble && original[pos] == tClose[0]) )
		{
			if ( temp.Length() > tLen )
			{
				temp.Chop(tLen);
				XStringW rep;
				if ( callback( context, temp, rep ) )
				{
					// bingo
					final.Chop( temp.Length() + tLen * 2 );
					final += rep;
					temp.ClearNoAlloc();
					count++;
				}
			}
		}
		prev = original[pos];
	}
	return count != 0;
}

PAPI void ParseCmdLineOptions( int n, const XString* opt, dvect<XString>* params, dvect<XStringKV>* options )
{
	for ( int i = 0; i < n; i++ )
	{
		if ( opt[i].Length() == 0 ) continue;
		if ( opt[i][0] == '-' )
		{
			if ( !options ) continue;
			int nm = 1;
			if ( opt[i].Length() >= 2 && opt[i][1] == '-' ) nm = 2;

			int eq = opt[i].Find( '=' );
			XStringKV kv;
			if ( eq > nm )
			{
				kv.Key = opt[i].Mid(nm, eq - nm);
				kv.Value = opt[i].Mid(eq+1);
			}
			else
			{
				kv.Key = opt[i].Mid(nm);
				kv.Value = "";
			}
			*options += kv;
		}
		else
		{
			if ( !params ) continue;
			*params += opt[i];
		}
	}
}

PAPI void ParseCmdLineOptionsARGV( int argc, wchar_t** argv, dvect<XString>* params, dvect<XStringKV>* options )
{
	dvect<XString> args;
	for ( int i = 1; i < argc; i++ )
		args += XString(argv[i]);
	return ParseCmdLineOptions( args.size(), &args[0], params, options );
}

PAPI void ParseCmdLineOptionsARGV( int argc, char** argv, dvect<XString>* params, dvect<XStringKV>* options )
{
	dvect<XString> args;
	for ( int i = 1; i < argc; i++ )
		args += XString(argv[i]);
	return ParseCmdLineOptions( args.size(), &args[0], params, options );
}

// All we do is escape the delimiter and backslashes
template< typename TStrRes, typename TStrSrc, typename TCH >
TStrRes TEncodeStringList( int n, const TStrSrc* list, TCH sep )
{
	TStrRes rs;
	if ( n == 0 ) return rs;

	for ( int i = 0; i < n; i++ )
	{
		// 1,2		  1\,2
		// 1,,2     1\,\,2
		// 1\,,2    1\\\,\,2
		const TCH* s = list[i];
		for ( int j = 0; s[j]; j++ )
		{
			if ( s[j] == '\\' )
			{
				rs += '\\';
				rs += '\\';
			}
			else if ( s[j] == sep )
			{
				rs += '\\';
				rs += s[j];
			}
			else rs += s[j];
		}
		rs += sep;
	}
	rs.Chop();
	return rs;
}

PAPI XStringA EncodeStringList( intp n, const XStringA* list, char sep )		{ return TEncodeStringList<XStringA, XStringA, char>( n, list, sep ); }
PAPI XStringW EncodeStringList( intp n, const XStringW* list, wchar_t sep )	{ return TEncodeStringList<XStringW, XStringW, wchar_t>( n, list, sep ); }

PAPI bool ParseStringListA( const XStringA& s, char sep, void* context, FStringAOut target )		{ return BaseParseStringList( s, sep, context, target ); }
PAPI bool ParseStringListW( const XStringW& s, wchar_t sep, void* context, FStringWOut target )		{ return BaseParseStringList( s, sep, context, target ); }


template<typename CH>
bool TSplitLines( const CH* src, uintp maxlen, bool (*consume)( void* cx, const CH* start, intp len ), void* cx )
{
	// Windows is		\r\n
	// Most are			\n
	// Mac OS 9 are		\r
	// We only support those three types
	const CH LF = 10;	// \n
	const CH CR = 13;	// \r
	uintp i = 0;
	uintp s = 0;
	bool more = true;
	while ( true )
	{
		if ( src[i] == 0 || i == maxlen )
		{
			if ( s != i ) more = consume( cx, src + s, i - s );
			break;
		}
		else if ( src[i] == LF )
		{
			more = consume( cx, src + s, 1 + i - s );
			s = i + 1;
		}
		else if ( src[i] == CR && i + 1 < maxlen && src[i+1] != 0 && src[i+1] != LF )
		{
			more = consume( cx, src + s, 1 + i - s );
			s = i + 1;
		}
		i++;
	}
	return more;
}

PAPI bool SplitLines( const char* src, uintp maxlen, bool (*consume)( void* cx, const char* start, intp len ), void* cx )		{ return TSplitLines( src, maxlen, consume, cx ); }
PAPI bool SplitLines( const wchar_t* src, uintp maxlen, bool (*consume)( void* cx, const wchar_t* start, intp len ), void* cx )	{ return TSplitLines( src, maxlen, consume, cx ); }

static bool ConsumeLineA( void* cx, const char* start, intp len )
{
	auto c = (podvec<XStringA>*) cx;
	c->push( XStringA(start, len) );
	return true;
}

static bool ConsumeLineW( void* cx, const wchar_t* start, intp len )
{
	auto c = (podvec<XStringW>*) cx;
	c->push( XStringW(start, len) );
	return true;
}

PAPI void SplitLines( const char* src, uintp maxlen, podvec<XStringA>& lines )		{ SplitLines( src, maxlen, ConsumeLineA, &lines ); }
PAPI void SplitLines( const wchar_t* src, uintp maxlen, podvec<XStringW>& lines )	{ SplitLines( src, maxlen, ConsumeLineW, &lines ); }
PAPI void SplitLines( const XStringA& src, podvec<XStringA>& lines )				{ SplitLines( src, -1, ConsumeLineA, &lines ); }
PAPI void SplitLines( const XStringW& src, podvec<XStringW>& lines )				{ SplitLines( src, -1, ConsumeLineW, &lines ); }


#define ISWHITE(c) (c == 32 || c == 9 || c == 10 || c == 13)

template<bool CaseSensitive, typename TChar>
PAPI int Tstrcmp_trimmed( const TChar* a, size_t a_size, const TChar* b, size_t b_size )
{
	static_assert( ( sizeof(TChar) == sizeof(char) ) || ( sizeof(TChar) == sizeof(wchar_t) ), "Type must be char or wchar_t" );
	int res = 0;
	while ( a_size && ISWHITE(*a) )
	{
		a++;
		a_size--;
	}

	const TChar* a_end = a + a_size - 1;
	while ( a_size && ISWHITE(*a_end) )
	{
		a_end--;
		a_size--;
	}

	while ( b_size && ISWHITE(*b) )
	{
		b++;
		b_size--;
	}

	const TChar* b_end = b + b_size - 1;
	while ( b_size && ISWHITE(*b_end) )
	{
		b_end--;
		b_size--;
	}

	size_t minsize = min(a_size, b_size);
	if ( sizeof(TChar) == sizeof(char) )
	{
		if ( CaseSensitive )
			res = strncmp( (const char*) a, (const char*) b, minsize );
		else
#ifdef _WIN32
			res = strnicmp( (const char*) a, (const char*) b, minsize );
#else
			res = strncasecmp( (const char*) a, (const char*) b, minsize );
#endif
	}
	else
	{
		if ( CaseSensitive )
			res = wcsncmp( (const wchar_t*) a, (const wchar_t*) b, minsize );
		else
#ifdef _WIN32
			res = wcsnicmp( (const wchar_t*) a, (const wchar_t*) b, minsize );
#else
			res = wcsncasecmp( (const wchar_t*) a, (const wchar_t*) b, minsize );
#endif
	} 
		

	if ( res || ( a_size == b_size ) )
		return res;
	else if ( a_size > b_size )
		return 1;
	else 
		return -1;
}

PAPI int wcscmp_trimmed(  const wchar_t* a, const wchar_t* b )									{ return Tstrcmp_trimmed<true, wchar_t>(  a, wcslen(a), b, wcslen(b) ); }
PAPI int wcsicmp_trimmed( const wchar_t* a, const wchar_t* b )									{ return Tstrcmp_trimmed<false, wchar_t>( a, wcslen(a), b, wcslen(b) ); }
PAPI int wcscmp_trimmed(  const wchar_t* a, size_t a_size, const wchar_t* b, size_t b_size )	{ return Tstrcmp_trimmed<true, wchar_t>(  a, a_size,    b, b_size ); }
PAPI int wcsicmp_trimmed( const wchar_t* a, size_t a_size, const wchar_t* b, size_t b_size )	{ return Tstrcmp_trimmed<false, wchar_t>( a, a_size,    b, b_size ); }

PAPI int strcmp_trimmed(  const char* a, const char* b )										{ return Tstrcmp_trimmed<true, char>(  a, strlen(a), b, strlen(b) ); }
PAPI int strcmpi_trimmed( const char* a, const char* b )										{ return Tstrcmp_trimmed<false, char>( a, strlen(a), b, strlen(b) ); }
PAPI int strcmp_trimmed(  const char* a, size_t a_size, const char* b, size_t b_size )			{ return Tstrcmp_trimmed<true, char>(  a, a_size,    b, b_size ); }
PAPI int strcmpi_trimmed( const char* a, size_t a_size, const char* b, size_t b_size )			{ return Tstrcmp_trimmed<false, char>( a, a_size,    b, b_size ); }
