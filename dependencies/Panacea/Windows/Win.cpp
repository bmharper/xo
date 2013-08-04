#include "pch.h"
#include "Win.h"

namespace Panacea
{
	namespace Win
	{
		void PAPI WindowsMessageDispatch( HWND wnd )
		{
			BOOL bRet;
			MSG msg;

			while( (bRet = GetMessage( &msg, wnd, 0, 0 )) != 0)
			{ 
				if (bRet == -1)
				{
					// handle the error and possibly exit
				}
				else
				{
					TranslateMessage(&msg); 
					DispatchMessage(&msg); 
				}
			}
		}
	}
}

HBITMAP AbcCreateDIBSection( int width, int height, void** bits )
{
	BITMAPINFO	bmpInfoLoc;
	HBITMAP		bmpSurface = NULL;
	memset(&bmpInfoLoc.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmpInfoLoc.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfoLoc.bmiHeader.biBitCount = 32;
	bmpInfoLoc.bmiHeader.biWidth = width;
	bmpInfoLoc.bmiHeader.biHeight = height;	// negative = top-down
	bmpInfoLoc.bmiHeader.biPlanes = 1;
	bmpInfoLoc.bmiHeader.biCompression = BI_RGB;
	bmpInfoLoc.bmiHeader.biSizeImage = 0;
	bmpInfoLoc.bmiHeader.biXPelsPerMeter = 72;
	bmpInfoLoc.bmiHeader.biYPelsPerMeter = 72;
	bmpInfoLoc.bmiHeader.biClrUsed = 0;
	bmpInfoLoc.bmiHeader.biClrImportant = 0;
	bmpSurface = CreateDIBSection( NULL, &bmpInfoLoc, DIB_RGB_COLORS, bits, NULL, NULL );
	return bmpSurface;
}


static HMODULE			HKTM;
static NTDLLFunctions	StaticNTDLL;

PAPI NTDLLFunctions* NTDLL()
{
	StaticNTDLL.EnsureInit();
	return &StaticNTDLL;
}

void NTDLLFunctions::EnsureInit()
{
	if ( IsInitialized )
		return;
	IsInitialized = true;

	OSVERSIONINFO ver;
	ver.dwOSVersionInfoSize = sizeof(ver);
	GetVersionEx( &ver );
	u32 v = ver.dwMajorVersion * 100 + ver.dwMinorVersion;
	AtLeastWin7 = v >= 601;
	AtLeastWinVista = v >= 600;

	HMODULE k;

#define GET(f) (FARPROC&) f = GetProcAddress(k, #f)

	if ( !HKTM )
		HKTM = LoadLibrary( L"Ktmw32.dll" );
	k = HKTM;

	if ( k )
	{
		GET(CreateTransaction);
		GET(CommitTransaction);
		GET(RollbackTransaction);
	}

	k = LoadLibrary( L"Kernel32.dll" );
	AbcCheckNULL(k);

	GET(CopyFileTransactedW);
	GET(GetFileAttributesTransactedW);
	GET(SetFileAttributesTransactedW);
	GET(DeleteFileTransactedW);
	GET(CreateDirectoryTransactedW);
	GET(RemoveDirectoryTransactedW);
	GET(FindFirstFileTransactedW);
	GET(CreateFileTransactedW);

#undef GET
}
