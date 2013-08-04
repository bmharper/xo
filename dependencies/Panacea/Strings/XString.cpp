#include "pch.h"
#include "XString.h"
#include "ConvertUTF.h"
#include "string.h"
#include "stdio.h"

#include <string>
#include <vector>

#define STRLEN strlen
#define STRCMP strcmp

XStringA XStringA::ToUtf8() const
{
	return ConvertHighAsciiToUTF8( *this );
}

void XStringW::ConvertFromUtf8( const char* utf8, int max_chars )
{
	XStringA src;
	src.MakeTemp( utf8, max_chars );
	*this = ConvertUTF8ToUTF16( src );
	src.DestroyTemp();
}

//XStringW XStringW::FromUtf8( const char* utf8, int max_chars )
//{
//	XStringA src( utf8, max_chars )
//	return ConvertUTF8ToUTF16( src );
//}

XStringA XStringW::ToUtf8() const
{
	return ConvertUTF16ToUTF8( *this );
}


int static_blank[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/*
XStringW XStringA::ToWide() const
{
	if ( length == 0 ) return L"";
	XStringW wide;
	wide.Resize( length );
	for ( int i = 0; i < length; i++ )
	{
		wide[i] = str[i];
	}
	return wide;
}
*/

void xstring_instantiate()
{
	using namespace std;

	string lo;
	XStringT<char> jack;

	XStringA hello;
	XStringW hello2;

	XStringA a, b, c;

	c = a + b;

	string f;
	c = f;
	f = c;

	XStringW w;
	wstring q;
	w = q;
	q = w;

	vector< XStringA > vec;
	Split( a, '+', vec );

	// implicit char -> wchar
	const char* pop = "hi";
	w = pop;

	// this should fail though
	//wchar_t mam = "no";
	//c = mam;
}

// make sure they compile
#ifdef _DEBUG
void testhashes()
{
	/*StrStrMap m1;
	StrPtrMap m2;
	IntStrMap m3;
	StrIntMap m4, m5, m6;
	m4.insert( "hello", 5 );
	m5 = m4;
	m4 = m5;
	m6 = m4; */

	/*CString aa;
	XStringT bb;

	bb = "asasa";
	aa = "qaewq";
	aa = bb;*/

}
#endif
