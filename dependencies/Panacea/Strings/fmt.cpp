#include "pch.h" 
#include "fmt.h"
#include "../Other/StackAllocators.h"
#include <stdarg.h>

inline void fmt_settype( char argbuf[128], intp pos, const char* width, char type )
{
	if ( width != NULL )
	{
		// set the type and the width specifier
		switch ( argbuf[pos - 1] )
		{
		case 'l':
		case 'h':
		case 'w':
			pos--;
			break;
		}

		for ( ; *width; width++, pos++ )
			argbuf[pos] = *width;

		argbuf[pos++] = type;
		argbuf[pos++] = 0;
	}
	else
	{
		// only set the type, not the width specifier
		argbuf[pos++] = type;
		argbuf[pos++] = 0;
	}
}

FMT_STRING fmtcore( const char* fmt, intp nargs, const fmtarg** args )
{
	// true if we have passed a %, and are looking for the end of the token
	intp tokenstart = -1;
	intp iarg = 0;
	bool no_args_remaining;
	bool spec_too_long;
	bool disallowed;

	char staticbuf[8192];
	AbCore::StackBuffer output( staticbuf );

	char argbuf[128];

#ifdef _WIN32
	const char* i64Prefix = "I64";
	const char* wcharPrefix = "";
	const char wcharType = 'S';
#else
	const char* i64Prefix = "ll";
	const char* wcharPrefix = "l";
	const char wcharType = 's';
#endif

	// we can always safely look one ahead, because 'fmt' is by definition zero terminated
	for ( intp i = 0; fmt[i]; i++ )
	{
		if ( tokenstart != -1 )
		{
			bool tokenint = false;
			bool tokenreal = false;
			switch ( fmt[i] )
			{
			case 'd':
			case 'i':
			case 'o':
			case 'u':
			case 'x':
			case 'X':
				tokenint = true;
			}

			switch ( fmt[i] )
			{
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
			case 'a':
			case 'A':
				tokenreal = true;
			}

			switch ( fmt[i] )
			{
			case 'a':
			case 'A':
			case 'c':
			case 'C':
			case 'd':
			case 'i':
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
			case 'H':
			case 'o':
			case 's':
			case 'S':
			case 'u':
			case 'x':
			case 'X':
			case 'p':
			case 'n':
			case 'v':
				no_args_remaining	= iarg >= nargs;								// more tokens than arguments
				spec_too_long		= i - tokenstart >= arraysize(argbuf) - 1;		// %_____too much data____v
				disallowed			= fmt[i] == 'n';
				if ( no_args_remaining || spec_too_long || disallowed )
				{
					for ( intp j = tokenstart; j <= i; j++ )
						output.AddItem( fmt[j] );
				}
				else
				{
					// prepare the single formatting token that we will send to snprintf
					intp argbufsize = 0;
					for ( intp j = tokenstart; j < i; j++ )
					{
						if ( fmt[j] == '*' ) continue;	// ignore
						argbuf[argbufsize++] = fmt[j];
					}

#define				SETTYPE1(type)			fmt_settype( argbuf, argbufsize, NULL, type )
#define				SETTYPE2(width, type)	fmt_settype( argbuf, argbufsize, width, type )

					// grow output buffer size until we don't overflow
					const fmtarg* arg = args[iarg];
					iarg++;
					intp outputSize = 1024;
					while ( true )
					{
						char* outbuf = (char*) output.Add( outputSize );
						bool done = false;
						int written = 0;
						switch ( arg->Type )
						{
						case fmtarg::TCStr:
							SETTYPE2("", 's');
							written = fmt_snprintf( outbuf, outputSize, argbuf, arg->CStr );
							break;
						case fmtarg::TWStr:
							SETTYPE2(wcharPrefix, wcharType);
							written = fmt_snprintf( outbuf, outputSize, argbuf, arg->CStr );
							break;
						case fmtarg::TI32:
							if (tokenint)	{ SETTYPE2("", fmt[i]); }
							else			{ SETTYPE2("", 'd'); }
							written = fmt_snprintf( outbuf, outputSize, argbuf, arg->I32 );
							break;
						case fmtarg::TU32:
							if (tokenint)	{ SETTYPE2("", fmt[i]); }
							else			{ SETTYPE2("", 'u'); }
							written = fmt_snprintf( outbuf, outputSize, argbuf, arg->UI32 );
							break;
						case fmtarg::TI64:
							if (tokenint)	{ SETTYPE2(i64Prefix, fmt[i]); }
							else			{ SETTYPE2(i64Prefix, 'd'); }
							written = fmt_snprintf( outbuf, outputSize, argbuf, arg->I64 );
							break;
						case fmtarg::TU64:
							if (tokenint)	{ SETTYPE2(i64Prefix, fmt[i]); }
							else			{ SETTYPE2(i64Prefix, 'u'); }
							written = fmt_snprintf( outbuf, outputSize, argbuf, arg->UI64 );
							break;
						case fmtarg::TDbl:
							if (tokenreal)	{ SETTYPE1(fmt[i]); }
							else			{ SETTYPE1('g'); }
							written = fmt_snprintf( outbuf, outputSize, argbuf, arg->Dbl );
							break;
						}
						if ( written >= 0 && written < outputSize )
						{
							output.MoveCurrentPos( written - outputSize );
							break;
						}
						// discard and try again with a larger buffer
						output.MoveCurrentPos( -outputSize );
						outputSize = outputSize * 2;
					}
#undef SETTYPE1
#undef SETTYPE2
				}
				tokenstart = -1;
				break;
			case '%':
				output.AddItem( '%' );
				tokenstart = -1;
				break;
			default:
				break;
			}
		}
		else
		{
			switch ( fmt[i] )
			{
			case '%':
				tokenstart = i;
				break;
			default:
				output.AddItem( fmt[i] );
				break;
			}
		}
	}
	output.AddItem( '\0' );
	return FMT_STRING( (const char*) output.Buffer );
}


PAPI FMT_STRING fmt( const char* fs )
{
	return fmtcore( fs, 0, NULL );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1 )
{
	const fmtarg* args[1] = {&a1};
	return fmtcore( fs, 1, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2 )
{
	const fmtarg* args[2] = {&a1, &a2};
	return fmtcore( fs, 2, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3 )
{
	const fmtarg* args[3] = {&a1, &a2, &a3};
	return fmtcore( fs, 3, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4 )
{
	const fmtarg* args[4] = {&a1, &a2, &a3, &a4};
	return fmtcore( fs, 4, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5 )
{
	const fmtarg* args[5] = {&a1, &a2, &a3, &a4, &a5};
	return fmtcore( fs, 5, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6 )
{
	const fmtarg* args[6] = {&a1, &a2, &a3, &a4, &a5, &a6};
	return fmtcore( fs, 6, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7 )
{
	const fmtarg* args[7] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7};
	return fmtcore( fs, 7, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8 )
{
	const fmtarg* args[8] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8};
	return fmtcore( fs, 8, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9 )
{
	const fmtarg* args[9] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9};
	return fmtcore( fs, 9, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10 )
{
	const fmtarg* args[10] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10};
	return fmtcore( fs, 10, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11 )
{
	const fmtarg* args[11] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10, &a11};
	return fmtcore( fs, 11, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12 )
{
	const fmtarg* args[12] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10, &a11, &a12};
	return fmtcore( fs, 12, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13 )
{
	const fmtarg* args[13] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10, &a11, &a12, &a13};
	return fmtcore( fs, 13, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13, const fmtarg& a14 )
{
	const fmtarg* args[14] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10, &a11, &a12, &a13, &a14};
	return fmtcore( fs, 14, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13, const fmtarg& a14, const fmtarg& a15 )
{
	const fmtarg* args[15] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10, &a11, &a12, &a13, &a14, &a15};
	return fmtcore( fs, 15, args );
}
PAPI FMT_STRING fmt( const char* fs, const fmtarg& a1, const fmtarg& a2, const fmtarg& a3, const fmtarg& a4, const fmtarg& a5, const fmtarg& a6, const fmtarg& a7, const fmtarg& a8, const fmtarg& a9, const fmtarg& a10, const fmtarg& a11, const fmtarg& a12, const fmtarg& a13, const fmtarg& a14, const fmtarg& a15, const fmtarg& a16 )
{
	const fmtarg* args[16] = {&a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10, &a11, &a12, &a13, &a14, &a15, &a16};
	return fmtcore( fs, 16, args );
}

static inline int fmt_translate_snprintf_return_value( int r, size_t count )
{
	if ( r < 0 || (size_t) r >= count )
		return -1;
	else
		return r;
}

PAPI int fmt_snprintf( char* destination, size_t count, const char* format_str, ... )
{
	va_list va;
	va_start( va, format_str );
	int r = vsnprintf( destination, count, format_str, va );
	va_end( va ); 
	return fmt_translate_snprintf_return_value( r, count );
}

PAPI int fmt_swprintf( wchar_t* destination, size_t count, const wchar_t* format_str, ... )
{
	va_list va;
	va_start( va, format_str );
	int r = vswprintf( destination, count, format_str, va );
	va_end( va ); 
	return fmt_translate_snprintf_return_value( r, count );
}

/*

-- lua script used to generate the functions

local api = "PAPI"
local maxargs = 16

function makefunc( decl, name, withbody, reps )
	local str = decl .. " FMT_STRING " .. name .. "( const char* fs"
	for i = 1, reps do
		str = str .. ", const fmtarg& a" .. tostring(i)
	end
	str = str .. " )"
	if withbody then
		str = str .. "\n"
		str = str .. "{\n"
		if reps ~= 0 then
			str = str .. "\tconst fmtarg* args[" .. tostring(reps) .. "] = {"
			for i = 1, reps do
				str = str .. "&a" .. tostring(i) .. ", "
			end
			str = str:sub(0, #str - 2)
			str = str .. "};\n"
			str = str .. "\treturn fmtcore( fs, " .. tostring(reps) .. ", args );\n";
		else
			str = str .. "\treturn fmtcore( fs, 0, NULL );\n";
		end
		str = str .. "}\n"
	else
		str = str .. ";\n"
	end

	return str
end

local body, header = "", ""
for lim = 0, maxargs do
	body = body .. makefunc(api, "fmt", true, lim)
	header = header .. makefunc(api, "fmt", false, lim)
end


print( header )
print( body )

*/

