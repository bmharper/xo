#include "pch.h"
#include "lmffile.h"

lmFFile::lmFFile(void)
{
	of = NULL;
}

lmFFile::lmFFile( FILE* f )
{
	of = f;
}

lmFFile::~lmFFile(void)
{
}

void lmFFile::SetFile( FILE* f )
{
	of = f;
}

size_t lmFFile::Read( void* dst, size_t bytes )
{
	return bytes * fread( dst, bytes, 1, of );
}

size_t lmFFile::Write( const void* src, size_t bytes )
{
	return bytes * fwrite( src, bytes, 1, of );
}

void lmFFile::Seek( size_t pos )
{
	_fseeki64( of, (long long) pos, SEEK_SET );
}

void lmFFile::SeekOffset( size_t pos )
{
	_fseeki64( of, (long long) pos, SEEK_CUR );
}

size_t lmFFile::Tell()
{
	return _ftelli64( of );
}

