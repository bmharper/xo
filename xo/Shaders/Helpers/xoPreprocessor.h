#pragma once

#include "../../xoDefs.h"

class XOAPI xoPreprocessor
{
public:
	friend struct _yycontext;

	void		SetMacro( const char* name, const char* value );
	void		ClearMacros();
	intp		MacroCount() { return Macros.size(); }
	xoString	Run( const char* raw );

protected:
	fhashmap<xoString, xoString>	Macros;

	static bool	Match( const char* buf, uintp bufPos, const xoString& find );
	static bool	IsIdentChar( char c );

	void		RunMacros( const char* raw, podvec<char>& out );
};