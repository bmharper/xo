#ifndef STRINGUTIL_H
#define STRINGUTIL_H


#include <string>
#include <algorithm>

namespace stdext
{
/*
template < class char_t >
void upcase_char( char_t& ch )
{
	if ( ch >= 'a' && ch <= 'z' ) ch += 'A' - 'a';
}

// I don't know how to templatize this properly.
// If I try and use upcase_char< string_t::value_type > I get an error.

template < class string_t >
string_t uppercase( const string_t& str )
{
	string_t copy = str;
	for_each( copy.begin(), copy.end(), upcase_char< char > );
	return copy;
}*/

	
	/*template < class C >
std::basic_string<C> upcase( const std::basic_string<C>& in )
{
	std::basic_string<C> copy;
	int len = (int) in.length();
	copy.resize( len );
	for (int i = 0; i < len; i++)
		copy[i] = toupper( in[i] );
	return copy;
} */

inline std::string upcase( const std::string& in )
{
	std::string copy;
	int len = (int) in.length();
	copy.resize( len );
	for (int i = 0; i < len; i++)
		copy[i] = toupper( in[i] );
	return copy;
} 

inline std::wstring upcase( const std::wstring& in )
{
	std::wstring copy;
	int len = (int) in.length();
	copy.resize( len );
	for (int i = 0; i < len; i++)
		copy[i] = toupper( in[i] );
	return copy;
} 

inline std::string lowcase( const std::string& in )
{
	std::string copy;
	int len = (int) in.length();
	copy.resize( len );
	for (int i = 0; i < len; i++)
		copy[i] = tolower( in[i] );
	return copy;
} 

inline std::wstring lowcase( const std::wstring& in )
{
	std::wstring copy;
	int len = (int) in.length();
	copy.resize( len );
	for (int i = 0; i < len; i++)
		copy[i] = tolower( in[i] );
	return copy;
} 

inline void replace( std::string& str, char src, char dst )
{
	std::string::iterator it;
	for (it = str.begin(); it != str.end(); it++)
		if ( *it == src ) *it = dst;
}

inline void replace( std::wstring& str, wchar_t src, wchar_t dst )
{
	std::wstring::iterator it;
	for (it = str.begin(); it != str.end(); it++)
		if ( *it == src ) *it = dst;
}

inline std::wstring make_wstring( wchar_t ch )
{
	std::wstring str;
	str.resize( 1 );
	str[0] = ch;
	return str;
}

inline std::string make_string( char ch )
{
	std::string str;
	str.resize( 1 );
	str[0] = ch;
	return str;
}

}
#endif
