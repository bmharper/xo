#include "pch.h"
#include "lmmemfile.h"

lmMemFile::lmMemFile(void)
{
	Reset();
}

void lmMemFile::Reset()
{
	FileSize = 0;
	Buff = NULL;
	BuffSize = 0;
	MaxSize = 0;
	Pos = 0;
	ManageMem = false;
	AllowGrow = false;
}

// Open for reading
lmMemFile::lmMemFile( void* readbuff, size_t bsize )
{
	Reset();
	OpenForRead( readbuff, bsize );
}

// Opens for writing. set maxsize = 0 for no limit
lmMemFile::lmMemFile( size_t maxsize )
{
	Reset();
	OpenForWrite( maxsize );
}

lmMemFile::~lmMemFile(void)
{
	Close();
}

void* lmMemFile::GetBuffer() const
{
	return Buff;
}

void lmMemFile::OpenForRead( void* buff, size_t bsize )
{
	Close();
	ManageMem = false;
	Buff = (unsigned char*) buff;
	FileSize = bsize;
	MaxSize = bsize;
	BuffSize = bsize;
	Pos = 0;
}

// set maxsize = 0 for no limit
void lmMemFile::OpenForWrite( size_t maxsize )
{
	Close();
	Buff = NULL;
	MaxSize = maxsize;
	BuffSize = 0;
	Pos = 0;
	FileSize = 0;
	ManageMem = true;
	AllowGrow = true;
}

void lmMemFile::Close()
{
	if (ManageMem && Buff)
		free(Buff);
	Buff = NULL;
	FileSize = 0;
	Pos = 0;
	MaxSize = 0;
	BuffSize = 0;
	AllowGrow = false;
}

size_t lmMemFile::Read( void* dst, size_t bytes )
{
	size_t rsize = min(BytesLeft(), bytes);
	if (rsize > 0)
		memcpy(dst, Buff + Pos, rsize);
	Pos += rsize;
	return rsize;
}


size_t lmMemFile::Write( const void* src, size_t bytes )
{
	size_t rsize = min(BytesLeft(), bytes);
	if (rsize < bytes && AllowGrow)
	{
		if (!Grow( bytes )) return 0;
		rsize = bytes;
	}

	if (rsize > 0)
		memcpy(Buff + Pos, src, rsize);
	FileSize = max(FileSize, Pos + rsize);
	Pos += rsize;
	return rsize;
}

void lmMemFile::Seek( size_t pos )
{
	Pos = pos;
}

void lmMemFile::SeekOffset( size_t pos )
{
	Pos += pos;
}

size_t lmMemFile::Tell()
{
	return Pos;
}

size_t lmMemFile::BytesLeft()
{
	if (Pos >= BuffSize) return 0;
	return BuffSize - Pos;
}

bool lmMemFile::Grow( size_t provision )
{
	size_t increase = max( provision * 2, (size_t) (512 * 1024) );
	if (MaxSize != 0)
	{
		if (increase + BuffSize > MaxSize)
			increase = MaxSize - BuffSize;
	}
	if (increase < provision)
		return false;

	if (Buff == NULL)
		Buff = (BYTE*) malloc(increase);
	else
	{
		BYTE* nbuf = (BYTE*) realloc(Buff, BuffSize + increase);
		AbcCheckAlloc(nbuf);
		Buff = nbuf;
	}
	BuffSize += increase;
	return Buff != NULL;
}

