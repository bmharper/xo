#include "pch.h"
#include "../xoDefs.h"
#include "../xoDoc.h"
#include "xoDocParser.h"

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
				Buf = (char*) xoMallocOrDie( Capacity );
				memcpy( Buf, StaticBuf, Len );
			}
			else
			{
				Capacity *= 2;
				Buf = (char*) xoReallocOrDie( Buf, Capacity );
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

xoString xoDocParser::Parse( const char* src, xoDomNode* target )
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
	pvect<xoDomNode*> stack;
	stack += target;

	auto err = [&]( const char* msg ) -> xoString
	{
		intp start = xoMax<intp>( pos - 1, 0 );
		xoString sample;
		sample.Set( src + start, 10 );
		return fmt( "Parse error at position %v (%v): %v", pos, sample.Z, msg );
	};

	auto newNode = [&]() -> xoString
	{
		if ( xEnd - xStart <= 0 ) return "Tag is empty";
		intp i = 0;
		for ( ; i < xoTagEND; i++ )
		{
			if ( EqNoCase(xoTagNames[i], src + xStart, xEnd - xStart) )
				break;
		}
		if ( i == xoTagEND )
			return xoString("Unrecognized tag ") + xoString( src + xStart, xEnd - xStart );
		stack += stack.back()->AddNode( (xoTag) i );
		return "";
	};

	auto newText = [&]() -> xoString
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
					else return fmt( "Invalid escape sequence (%v)", xoString(src + escape, len).Z );
					escape = -1;
				}
			}
			else
			{
				bool w = IsWhite(c);
				if ( c == '&' )				escape = i + 1;
				else if ( w && white )		{}					// trim leading whitespace
				else if ( c == '\r' )		{}					// ignore '\r'
				else						str.Add( c );
				white = white && w;
			}
		}
		if ( escape != -1 )
			return "Unfinished escape sequence";
		// trim trailing whitespace
		while ( str.Len != 0 && IsWhite(str.Buf[str.Len - 1]) )
			str.Len--;
		if ( str.Len != 0 )
		{
			str.Terminate();
			stack.back()->AddText( str.Buf );
		}
		return "";
	};

	auto closeNodeCompact = [&]() -> xoString
	{
		if ( stack.size() == 1 )
			return "Too many closing tags"; // not sure if this is reachable; suspect not.
		stack.pop();
		return "";
	};

	auto closeNode = [&]() -> xoString
	{
		if ( stack.size() == 1 )
			return "Too many closing tags";
		xoDomNode* top = stack.back();
		if ( !EqNoCase(xoTagNames[top->GetTag()], src + xStart, xEnd - xStart) )
			return fmt( "Cannot close %v here. Expected %v close.", xoString(src + xStart, xEnd - xStart).Z, xoTagNames[top->GetTag()] );
		stack.pop();
		return "";
	};

	auto setAttrib = [&]() -> xoString
	{
		if ( xEnd - xStart <= 0 ) return "Attribute name is empty"; // should be impossible to reach this, due to possible state transitions
		
		xoDomNode* node = stack.back();
		char buf[64];

		intp bodyStart = xEnd + 2;
		if ( EqNoCase("style", src + xStart, xEnd - xStart) )
		{
			if ( node->StyleParse( src + bodyStart, pos - bodyStart ) )
				return "";
			return xoString( "Invalid style: " ) + xoString( src + bodyStart, pos - bodyStart );
		}
		else if ( EqNoCase("class", src + xStart, xEnd - xStart) )
		{
			intp cstart = bodyStart;
			for ( intp i = bodyStart; i <= pos; i++ )
			{
				if ( i == pos || IsWhite(src[i]) )
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
							node->AddClass( xoString(src + cstart, i - cstart).Z );
						}
					}
					cstart = i + 1;
				}
			}
			return "";
		}

		return xoString("Unrecognized attribute ") + xoString( src + xStart, xEnd - xStart );
	};

	for ( ; src[pos]; pos++ )
	{
		int c = src[pos];
		xoString e;
		switch ( s )
		{
		case SText:
			if ( c == '<' )						{ s = STagOpen; xStart = pos + 1; e = newText(); break; }
			else								{ break; }
		case STagOpen:
			if ( IsWhite(c) )					{ s = SAttribs; xEnd = pos; e = newNode(); break; }
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
			if ( IsWhite(c) )					{ break; }
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

bool xoDocParser::IsWhite( int c )
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool xoDocParser::IsAlpha( int c )
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool xoDocParser::EqNoCase( const char* a, const char* b, intp bLen )
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
	XOASSERTDEBUG(false);
	return false;
}
