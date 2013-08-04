#include "pch.h"
#include "MMFile.h"

// NOTE: If you implement this on linux, do not forget to add msync() before munmap()

AbcMMFile::AbcMMFile()
{
#ifdef _WIN32
	File = NULL;
	Map = NULL;
#endif
	Data = NULL;
}

AbcMMFile::~AbcMMFile()
{
	Close();
}

void AbcMMFile::Close()
{
#ifdef _WIN32
	if ( Data ) UnmapViewOfFile(Data);
	ZClose(Map);
	ZClose(File);
#endif
	Data = NULL;
}

#ifdef _WIN32
void AbcMMFile::ZClose(HANDLE& h)
{
	if ( h ) CloseHandle(h);
	h = NULL;
}
#endif

bool AbcMMFile::Open( LPCWSTR path, u64 size, bool write )
{
	Close();
#ifdef _WIN32
	File = CreateFile( path, GENERIC_READ | (write ? GENERIC_WRITE : 0), FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, NULL );		if (!File)	{Close(); return false;}
	Map = CreateFileMapping( File, NULL, write ? PAGE_READWRITE : PAGE_READONLY, size >> 32, size, NULL );										if (!Map)	{Close(); return false;}
	Data = (u8*) MapViewOfFile( Map, write ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size );														if (!Data)	{Close(); return false;}
	return true;
#endif
}
