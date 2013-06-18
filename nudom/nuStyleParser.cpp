#include "pch.h"
#include "nuStyleParser.h"

/*

This parser is based on the concept of prioritized symbols, an idea I learned from PEGs.

.my-style {
	padding:	50px;
	width:		10pt;
}

*/

// The CHOOSE functions will try the items in order, and return true if any of them matched.
// If none matched, then the parse position is reset to its original value

#define CURRENT			(Src[Pos])
#define CHOOSE1(func1)	{ PushChoose(); if (!PopChoose(func1)) return false; }

void nuStyleParser::Reset()
{
	Error = "";
	Pos = 0;
}

bool nuStyleParser::Parse( const char* src )
{
	Reset();
	return _Root();
}

bool nuStyleParser::_Root()
{
	while ( !AtEnd() )
	{
		_WS();
		CHOOSE1(_Class());
	}
	return true;
}

void nuStyleParser::PushChoose()
{
	ChooseStack.push( Pos );
}

bool nuStyleParser::PopChoose( bool ok )
{
	if ( !ok )
		Error = fmt( "Fail at position %v", ChooseStack.back() );
	ChooseStack.pop();
	return ok;
}

bool nuStyleParser::AtEnd()
{
	return CURRENT == 0;
}

bool nuStyleParser::_Class()
{
	CHOOSE1(_ClassName());
	_WS();
	CHOOSE1(_CHAR('{'));
	_WS();
	while ( !AtEnd() )
	{
		CHOOSE1(_AttribName());
		_WS();
		CHOOSE1(_AttribValue());
	}
	CHOOSE1(_CHAR('}'));
	return true;
}

bool nuStyleParser::_ClassName()
{
	if ( CURRENT != '.' ) return false;
	return true;
}

bool nuStyleParser::_AttribName()
{
	int orgPos = Pos;
	while ( !AtEnd() )
	{
		int c = CURRENT;
		if (	(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				c == '-' )
		{
			Pos++;
		}
	}
	if ( Pos == orgPos ) return false;
	if ( CURRENT != ':' ) return false;
	return true;
}

bool nuStyleParser::_AttribValue()
{
	return true;
}

bool nuStyleParser::_WS()
{
	int orgPos = Pos;
	while ( !AtEnd() )
	{
		int c = CURRENT;
		if ( c == 32 || c == 9 || c == 10 || c == 13 )
			Pos++;
		else
			break;
	}
	return Pos != orgPos;
}

bool nuStyleParser::_CHAR( char ch )
{
	if ( CURRENT != ch ) return false;
	Pos++;
	return true;
}
