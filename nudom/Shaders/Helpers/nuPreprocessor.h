#pragma once

#include "nuDefs.h"

class NUAPI nuPreprocessor
{
public:
	friend struct _yycontext;

	void		SetMacro( const char* name, const char* value );
	void		ClearMacros();
	intp		MacroCount() { return Macros.size(); }
	nuString	Run( const char* raw );

protected:
	fhashmap<nuString, nuString>	Macros;

	static bool	Match( const char* buf, uintp bufPos, const nuString& find );
	static bool	IsIdentChar( char c );

	void		RunMacros( const char* raw, podvec<char>& out );
};