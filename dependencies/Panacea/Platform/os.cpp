#include "pch.h"
#include "os.h"
#include "../Windows/Registry.h"

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static LPFN_ISWOW64PROCESS	fnIsWow64Process = NULL;
static int					CachedIsWow64Process = 0;

PAPI bool AbcOSWinIsWow64()
{
	static const int IsWOW64 = 1;
	static const int NotWOW64 = -1;

	if ( CachedIsWow64Process == IsWOW64 )
		return true;
	else if ( CachedIsWow64Process == NotWOW64 )
		return false;

	BOOL bIsWow64 = FALSE;
	if ( !fnIsWow64Process )
	{
		HMODULE kernel = GetModuleHandle(L"kernel32");
		AbcAssert(kernel);
		fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress( kernel, "IsWow64Process");
	}
	if ( fnIsWow64Process )
	{
		if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
			CachedIsWow64Process = (bIsWow64 == TRUE) ? IsWOW64 : NotWOW64;
		else
			CachedIsWow64Process = NotWOW64;
	}
	else
	{
		CachedIsWow64Process = NotWOW64;
	}
	return CachedIsWow64Process == IsWOW64;
}

void AbcOSWinGetInfo( int* ver, bool* is64, int* dotNet, char* video )
{
	OSVERSIONINFOEX version;
	version.dwOSVersionInfoSize = sizeof(version);
	GetVersionEx( (OSVERSIONINFO*) &version );
	if ( ver ) *ver = version.dwMajorVersion * 100 + version.dwMinorVersion;
#ifdef _M_X64
	if ( is64 ) *is64 = true;
#else
	if ( is64 ) *is64 = AbcOSWinIsWow64();
#endif
	if ( dotNet )
	{
		const int NV = 4;
		const TCHAR *v11 = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v1.1.4322");
		const TCHAR *v20 = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v2.0.50727");
		const TCHAR *v30 = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v3.0");
		const TCHAR *v35 = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v3.5");
		const TCHAR* all[NV] = { v11, v20, v30, v35 };
		*dotNet = 0;
		int versions[NV] = { 11, 20, 30, 35 };
		for ( int i = 0; i < NV; i++ )
		{
			DWORD dw = Win::Registry::ReadDword( HKEY_LOCAL_MACHINE, all[i], L"Install" );
			if ( dw == 1 )
				*dotNet = versions[i];
		}
	}
	if ( video )
	{
		video[0] = 0;
		DISPLAY_DEVICE dev;
		dev.cb = sizeof(dev);
		for ( int nd = 0; EnumDisplayDevices( NULL, nd, &dev, 0 ); nd++ )
		{
			bool isPrim = 0 != (dev.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE);
			if ( isPrim )
			{
				int i;
				for ( i = 0; i < 128 && dev.DeviceString[i] != 0; i++ )
					video[i] = dev.DeviceString[i];
				if ( i == 128 ) i = 127;
				video[i] = 0;
				break;
			}
			dev.cb = sizeof(dev);
		}
	}
}

