#pragma once

#include "..\Other\JobQueue.h"
#include "VirtualFile.h"

/* Touch specified regions of a file.
This is intended to be run in a low priority thread.
Be sure to give this tool its own file handle, because the seek position on file handles is a shared resource.
*/
class PAPI AbcBackgroundFileLoader : public AbcJob
{
public:
	AbCore::IFile*		File;
	u64					Start;		// First byte to touch
	u64					Length;		// Length of region to touch
	u64					Pos;		// Current position
	u32					ChunkSize;
	u32					ChunkTimeMS;
	bool				Trace;

	static bool WholeFileByPathWait( LPCWSTR path );
	static bool WholeFileByPath( LPCWSTR path, AbcJobQueue* queue, double pause_seconds = 0 );
	static void WholeFileByHandle( AbCore::IFile* file, AbcJobQueue* queue, double pause_seconds = 0 );

	void Initialize( AbCore::IFile* file, u64 start, u64 len, double pause_seconds );

	AbcBackgroundFileLoader();
	virtual void Run();

};
