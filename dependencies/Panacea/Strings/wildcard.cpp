#include "pch.h"

// Base on code by By M Shahid Shafiq, from code project
template<typename CH>
static bool pattern_match(const CH *str, const CH *pattern, bool caseSense)
{
    enum State {
        Exact,      	// exact match
        Any,        	// ?
        AnyRepeat    	// *
    };

    const CH *s = str;
    const CH *p = pattern;
    const CH *q = 0;
    int state = 0;

#define EQUALS(a,b) (caseSense ? (a) == (b) : tolower(a) == tolower(b))

    bool match = true;
    while ( match && *p )
	{
        if ( *p == '*' )
		{
            state = AnyRepeat;
            q = p+1;
        }
		else if (*p == '?')
			state = Any;
        else state = Exact;

        if (*s == 0) break;

		switch (state)
		{
		case Exact:
			match = EQUALS(*s, *p);
			s++;
			p++;
			break;

		case Any:
			match = true;
			s++;
			p++;
			break;

		case AnyRepeat:
			match = true;
			s++;

			if (EQUALS(*s, *q)) p++;
			break;
		}
    }

    if (state == AnyRepeat) return EQUALS(*s, *q);
    else if (state == Any) return EQUALS(*s, *p);
    else return match && EQUALS(*s, *p);
} 

PAPI bool MatchWildcard( const char* str, const char* exp, bool casesense )
{
	return pattern_match( str, exp, casesense );
}

PAPI bool MatchWildcard( const wchar_t* str, const wchar_t* exp, bool casesense )
{
	return pattern_match( str, exp, casesense );
}
