#pragma once

#include "nuString.h"
#include "nuStyle.h"

class NUAPI nuStyleParser
{
public:
	nuString		Error;				// Blank if there was no error
	nuStyleTable	Table;				// Output

	bool	Parse( const char* src );	// 'src' must be null terminated. Returns true if parse succeeded.

protected:
	const char*		Src;
	int				Pos;

	podvec<int>		ChooseStack;		// Used by the CHOOSE macros

	void	PushChoose();
	bool	PopChoose( bool ok );

	void	Reset();
	bool	AtEnd();

	bool	_Root();
	bool	_Class();
	bool	_ClassName();
	bool	_AttribName();
	bool	_AttribValue();
	bool	_WS();
	bool	_CHAR( char ch );
};