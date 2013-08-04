#pragma once
#include "lmivfile.h"

class PAPI lmMemFile :	public lmIVFile
{
public:
	lmMemFile(void);
	lmMemFile( void* readbuff, size_t bsize );
	lmMemFile( size_t maxsize );
	virtual ~lmMemFile(void);

	void OpenForRead( void* buff, size_t bsize );
	void OpenForWrite( size_t maxsize );
	size_t	Size() const { return FileSize; }
	void* GetBuffer() const;
	void Close();


	virtual size_t	Read( void* dst, size_t bytes );
	virtual size_t	Write( const void* src, size_t bytes );
	virtual void	Seek( size_t pos );
	virtual void	SeekOffset( size_t pos );
	virtual size_t	Tell();

protected:
	void Reset();
	bool Grow( size_t provision );
	size_t BytesLeft();
	size_t Pos;
	size_t BuffSize;
	size_t MaxSize;
	size_t FileSize;
	bool AllowGrow;
	bool ManageMem;
	unsigned char* Buff;
};
