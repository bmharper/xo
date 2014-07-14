#include "pch.h"
#include "../nuDefs.h"
#include "../nuDoc.h"
#include "nuDocParser.h"

struct TempString
{
	intp	Capacity;
	intp	Len;
	char*	Buf;
	char	StaticBuf[256];

	TempString()
	{
		Capacity = arraysize(StaticBuf);
		Len = 0;
		Buf = StaticBuf;
		Buf[0] = 0;
	}
	~TempString()
	{
		if ( Buf != StaticBuf )
			free( Buf );
	}
	void Add( char c )
	{
		if ( Len == Capacity - 1 )
		{
			if ( Capacity == arraysize(StaticBuf) )
			{
				Capacity *= 2;
				Buf = (char*) nuMallocOrDie( Capacity );
				memcpy( Buf, StaticBuf, Len );
			}
			else
			{
				Capacity *= 2;
				Buf = (char*) nuReallocOrDie( Buf, Capacity );
			}
		}
		Buf[Len++] = c;
	}
	void Terminate()
	{
		Buf[Len] = 0;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nuString nuDocParser::Parse( const char* src, nuDomNode* target )
{
	enum States
	{
		SText,
		STagOpen,
		STagClose,
		SCompactClose,
		SAttribs,
		SAttribName,
		SAttribBodyStart,
		SAttribBodySingleQuote,
		SAttribBodyDoubleQuote,
	};

	States s = SText;
	intp pos = 0;
	intp xStart = 0;
	intp xEnd = 0;
	intp txtStart = 0;
	pvect<nuDomNode*> stack;
	stack += target;

	auto err = [&]( const char* msg ) -> nuString
	{
		intp start = nuMax<intp>( pos - 1, 0 );
		nuString sample;
		sample.Set( src + start, 10 );
		return fmt( "Parse error at position %v (%v): %v", pos, sample.Z, msg );
	};

	auto newNode = [&]() -> nuString
	{
		if ( xEnd - xStart <= 0 ) return "Tag is empty";
		intp i = 0;
		for ( ; i < nuTagEND; i++ )
		{
			if ( EqNoCase(nuTagNames[i], src + xStart, xEnd - xStart) )
				break;
		}
		if ( i == nuTagEND )
			return nuString("Unrecognized tag ") + nuString( src + xStart, xEnd - xStart );
		stack += stack.back()->AddNode( (nuTag) i );
		return "";
	};

	auto newText = [&]() -> nuString
	{
		bool white = true;
		intp escape = -1;
		TempString str;
		for ( intp i = txtStart; i < pos; i++ )
		{
			int c = src[i];
			if ( escape != -1 )
			{
				if ( c == ';' )
				{
					intp len = i - escape;
					if (		len == 2 && src[escape] == 'l' && src[escape + 1] == 't' ) str.Add( '<' );
					else if (	len == 2 && src[escape] == 'g' && src[escape + 1] == 't' ) str.Add( '>' );
					else if (	len == 3 && src[escape] == 'a' && src[escape + 1] == 'm' && src[escape + 2] == 'p' ) str.Add( '&' );
					else return fmt( "Invalid escape sequence (%v)", nuString(src + escape, len).Z );
					escape = -1;
				}
			}
			else
			{
				bool w = IsWhiteText(c);
				if ( c == '&' )				escape = i + 1;
				else if ( w && !white )		str.Add( ' ' );
				else if ( !w )				str.Add( c );
				white = w;
			}
		}
		if ( escape != -1 )
			return "Unfinished escape sequence";
		if ( str.Len != 0 )
		{
			str.Terminate();
			stack.back()->AddText( str.Buf );
		}
		return "";
	};

	auto closeNodeCompact = [&]() -> nuString
	{
		if ( stack.size() == 1 )
			return "Too many closing tags"; // not sure if this is reachable; suspect not.
		stack.pop();
		return "";
	};

	auto closeNode = [&]() -> nuString
	{
		if ( stack.size() == 1 )
			return "Too many closing tags";
		nuDomNode* top = stack.back();
		if ( !EqNoCase(nuTagNames[top->GetTag()], src + xStart, xEnd - xStart) )
			return fmt( "Cannot close %v here. Expected %v close.", nuString(src + xStart, xEnd - xStart).Z, nuTagNames[top->GetTag()] );
		stack.pop();
		return "";
	};

	auto setAttrib = [&]() -> nuString
	{
		if ( xEnd - xStart <= 0 ) return "Attribute name is empty"; // should be impossible to reach this, due to possible state transitions
		
		nuDomNode* node = stack.back();
		char buf[64];

		intp bodyStart = xEnd + 2;
		if ( EqNoCase("style", src + xStart, xEnd - xStart) )
		{
			if ( node->StyleParse( src + bodyStart, pos - bodyStart ) )
				return "";
			return "Invalid style";
		}
		else if ( EqNoCase("class", src + xStart, xEnd - xStart) )
		{
			intp cstart = bodyStart;
			for ( intp i = bodyStart; i != pos; i++ )
			{
				if ( IsWhiteNonText(src[i]) || i == pos - 1 )
				{
					intp len = i - cstart;
					if ( len > 0 )
					{
						if ( len < arraysize(buf) - 1 )
						{
							memcpy( buf, src + cstart, len );
							buf[len] = 0;
							node->AddClass( buf );
						}
						else
						{
							node->AddClass( nuString(src + cstart, i - cstart).Z );
						}
					}
					cstart = i + 1;
				}
			}
			return "";
		}

		return nuString("Unrecognized attribute ") + nuString( src + xStart, xEnd - xStart );
	};

	for ( ; src[pos]; pos++ )
	{
		int c = src[pos];
		nuString e;
		switch ( s )
		{
		case SText:
			if ( c == '<' )						{ s = STagOpen; xStart = pos + 1; e = newText(); break; }
			else								{ break; }
		case STagOpen:
			if ( IsWhiteNonText(c) )			{ s = SAttribs; xEnd = pos; e = newNode(); break; }
			else if ( c == '/' )				{ s = STagClose; xStart = pos + 1; break; }
			else if ( c == '>' )				{ s = SText; txtStart = pos + 1; xEnd = pos; e = newNode(); break; }
			else if ( IsAlpha(c) )				{ break; }
			else								{ return err( "Expected a tag name" ); }
		case STagClose:
			if ( c == '>' )						{ s = SText; txtStart = pos + 1; xEnd = pos; e = closeNode(); break; }
			else if ( IsAlpha(c) )				{ break; }
			else								{ return err( "Expected >" ); }
		case SCompactClose:
			if ( c == '>' )						{ s = SText; txtStart = pos + 1; e = closeNodeCompact(); break; }
			else								{ return err( "Expected >" ); }
		case SAttribs:
			if ( IsWhiteNonText(c) )			{ break; }
			else if ( IsAlpha(c) )				{ s = SAttribName; xStart = pos; break; }
			else if ( c == '/' )				{ s = SCompactClose; break; }
			else if ( c == '>' )				{ s = SText; txtStart = pos + 1; break; }
			else								{ return err( "Expected attributes or >" ); }
		case SAttribName:
			if ( IsAlpha(c) )					{ break; }
			else if ( c == '=' )				{ xEnd = pos; s = SAttribBodyStart; break; }
			else								{ return err( "Expected attribute name or =" ); }
		case SAttribBodyStart:
			if ( c == '\'' )					{ s = SAttribBodySingleQuote; break; }
			if ( c == '"' )						{ s = SAttribBodyDoubleQuote; break; }
			else 								{ return err( "Expected \"" ); }
		case SAttribBodySingleQuote:
			if ( c == '\'' )					{ s = SAttribs; e = setAttrib(); break; }
		case SAttribBodyDoubleQuote:
			if ( c == '"' )						{ s = SAttribs; e = setAttrib(); break; }
		}
		if ( e != "" )
			return err( e.Z );
	}

	if ( s != SText )
		return err( "Unfinished" );

	if ( stack.size() != 1 )
		return err( "Unclosed tags" );

	return "";
}

bool nuDocParser::IsWhiteNonText( int c )
{
	return c == 32 || c == 9 || c == 10 || c == 13;
}

bool nuDocParser::IsWhiteText( int c )
{
	return c == 32 || c == 9;
}

bool nuDocParser::IsAlpha( int c )
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool nuDocParser::EqNoCase( const char* a, const char* b, intp bLen )
{
	for ( intp i = 0; ; i++ )
	{
		if ( a[i] == 0 && i == bLen ) return true;
		if ( a[i] == 0 && i < bLen ) return false;
		if ( i == bLen ) return false;
		int aa = a[i] >= 'a' && a[i] <= 'z' ? a[i] + 'A' - 'a' : a[i];
		int bb = b[i] >= 'a' && b[i] <= 'z' ? b[i] + 'A' - 'a' : b[i];
		if ( aa != bb ) return false;
	}
	// should be unreachable
	NUASSERTDEBUG(false);
	return false;
}
