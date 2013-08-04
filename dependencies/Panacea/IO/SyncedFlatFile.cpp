#include "pch.h"
#include "SyncedFlatFile.h"
#include "Path.h"

AbcSyncedFlatFile::AbcSyncedFlatFile()  {}
AbcSyncedFlatFile::~AbcSyncedFlatFile() {}

void AbcSyncedFlatFile::Init( XString path )
{
	Path = path;
	Invalidate();
}

bool AbcSyncedFlatFile::IsOutOfDate()
{
	AbcDate twrite;
	u64 fsize = 0;
	PanPath_GetFileTimes( Path, NULL, NULL, &twrite, &fsize );
	return fsize != LastSize || twrite != LastWrite;
}

void AbcSyncedFlatFile::SetSynchronized()
{
	Invalidate();
	PanPath_GetFileTimes( Path, NULL, NULL, &LastWrite, &LastSize );
}

void AbcSyncedFlatFile::Invalidate()
{
	LastWrite = AbcDate();
	LastSize = -1;
}
