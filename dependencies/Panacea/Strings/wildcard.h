#ifndef WILDCARD_H_INCLUDED
#define WILDCARD_H_INCLUDED 1

PAPI bool MatchWildcard( const char* str, const char* exp, bool casesense );
PAPI bool MatchWildcard( const wchar_t* str, const wchar_t* exp, bool casesense );

#endif