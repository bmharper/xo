#pragma once

#include "../Strings/XString.h"

PAPI XString	AbcDebug_ReadAppValueStr( LPCSTR name );
PAPI bool		AbcDebug_ReadAppValue( LPCSTR name, double& v );
PAPI bool		AbcDebug_ReadAppValue( LPCSTR name, float& v );
PAPI XString	AbcSysErrorMsg( DWORD err );
PAPI XString	AbcSysLastErrorMsg();
PAPI void		AbcOutputDebugString( LPCSTR str );
PAPI void		AbcDebugBreak();

