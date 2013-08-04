#include "pch.h"
#include "libmath.h"

#ifdef _WIN32
#include <intrin.h>
#include <io.h>
#include <conio.h>
#endif


#ifdef _WIN32

namespace AbCore
{

PAPI void SetConsoleDimensions( int screen_width, int screen_height, int virtual_height )
{
	HANDLE hOut;
	CONSOLE_SCREEN_BUFFER_INFO SBInfo;
	hOut = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo( hOut, &SBInfo );
	COORD s;
	s.X = screen_width;
	s.Y = virtual_height;
	SetConsoleScreenBufferSize( hOut, s );
	SMALL_RECT r;
	r.Left = 0;
	r.Top = 0;
	r.Right = s.X - 1;
	r.Bottom = screen_height;
	SetConsoleWindowInfo( hOut, true, &r );
}

#pragma warning( push )
#pragma warning( disable: 4748 )	// /GS can not protect from buffer overruns etc etc
#pragma optimize( "", off )
__declspec(noinline) void PushStackOut( int bytes )
{
	const size_t FrameSize = 16; // this is a guess. I don't really know what it is.. x64 or x86
	const size_t SS = 1024;
	char buff[SS];
	memset( buff, 0x01, SS );
	bytes -= SS + FrameSize;
	if ( bytes > 0 ) PushStackOut( bytes );
}
#pragma warning( pop )

PAPI void FillStackWith( size_t bytesOfStackToFill, DWORD fillWith )
{
	char* self = (char*) _AddressOfReturnAddress();

	// Ensure that the stack pages have been committed
	PushStackOut( (int) bytesOfStackToFill );
	
	int skip = 1024;
	size_t ffill = bytesOfStackToFill - skip * 3;
	DWORD* dself = (DWORD*) (self - (bytesOfStackToFill - skip * 2));
	DWORD nw = DWORD(ffill / 4);
	for ( uint i = 0; i < nw; i++ )
		dself[i] = fillWith;

}
#pragma optimize( "", on )


PAPI void RedirectIOToConsole()
{
	int hConHandle;
	HANDLE lStdHandle;
	//CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	AllocConsole();


	// redirect unbuffered STDOUT to the console
	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle( (intptr_t) lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t) lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t) lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	//ios::sync_with_stdio();
}

}

#endif
