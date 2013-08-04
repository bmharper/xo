#include "pch.h"
#include "PagedFile.h"
#include "../Platform/mmap.h"

namespace AbCore {

PagedFile::PagedFile()
{
	Raw = NULL;
	WholeBuf = NULL;
	Init( NULL, 0 );
}

PagedFile::~PagedFile()
{
	Sleep();
}

void PagedFile::Reset()
{
	Sleep();
	Raw = NULL;
	CachedLength = 0;
}

void PagedFile::Init( AbCore::IFile* raw, uint pageBits )
{
	Pos = 0;
	Raw = raw;
	PageBits = pageBits;
	PageSize = 1 << pageBits;
	PageMask = PageSize - 1;
	CachedLength = raw ? raw->Length() : 0;
	ASSERT( WholeBuf == NULL );
	for ( int i = 0; i < NPages; i++ ) Pages[i].Reset();
}

void PagedFile::Sleep()
{
	for ( int i = 0; i < NPages; i++ )
	{
		ASSERT(!Pages[i].Dirty);
		Pages[i].Reset();
	}
	if ( WholeBuf )
	{
		free( WholeBuf );
		WholeBuf = NULL;
	}
}

bool PagedFile::Error()
{
	return Raw->Error();
}

XString PagedFile::ErrorMessage()
{
	return Raw->ErrorMessage();
}

void PagedFile::ClearError()
{
	Raw->ClearError();
}

bool PagedFile::Flush( int flags )
{
	bool ok = true;
	for ( int i = 0; i < NPages; i++ )
		ok = ok && RetirePage(i);
	Sleep();
	return ok && Raw->Flush( flags );
}
		
template<bool TWrite>
size_t PagedFile::ReadOrWrite( void* data, size_t bytes )
{
	Wake();
	size_t remain = bytes;
	byte* bdata = (byte*) data;
	for ( uint64 p = Pos & ~PageMask; remain != 0; p += PageSize )
	{
		int n = GetPage(p);
		if ( TWrite )
			Pages[n].Dirty = true;
		uint32 innerPos = uint32(Pos - p);
		uint32 dobytes = uint32(min((uint32) remain, PageSize - innerPos));
		if ( TWrite )
			memcpy( Pages[n].Buf + innerPos, bdata, dobytes );
		else
			memcpy( bdata, Pages[n].Buf + innerPos, dobytes );
		remain -= dobytes;
		bdata += dobytes;
		Pos += dobytes;
		if ( TWrite )
			CachedLength = max(CachedLength, Pos);
	}
	return bytes; // error leak
}

size_t PagedFile::Write( const void* data, size_t bytes )	{ return ReadOrWrite<true>( const_cast<void*>(data), bytes ); }
size_t PagedFile::Read( void* data, size_t bytes )			{ return ReadOrWrite<false>( data, bytes ); }

bool PagedFile::SetLength( UINT64 len )
{
	bool r = Raw->SetLength(len);
	if (r)
		CachedLength = len;
	return r;
}

UINT64 PagedFile::Position()
{
	return Pos;
}

UINT64 PagedFile::Length()
{
	return CachedLength;
}

bool PagedFile::Seek( UINT64 pos )
{
	Pos = pos;
	return true;
}

void PagedFile::Wake()
{
	if ( WholeBuf == NULL )
	{
		WholeBuf = AbcMallocOrDie( NPages * PageSize );
		for ( int i = 0; i < NPages; i++ )
		{
			Pages[i].Pos = -1;
			Pages[i].Dirty = false;
			Pages[i].Buf = ((byte*) WholeBuf) + PageSize * i;
		}
	}
}

int PagedFile::GetPage( uint64 pos )
{
	for ( int i = 0; i < NPages; i++ )
	{
		if ( Pages[i].Pos == pos )
			return i;
	}
	return AcquirePage( pos );
}

bool PagedFile::RetirePage( int n )
{
	if ( Pages[n].Dirty )
	{
		Pages[n].Dirty = false;
		return Raw->WriteAt( Pages[n].Pos, Pages[n].Buf, PageSize ) == PageSize;
	}
	else
		return true;
}

// Place page n at slot 0
void PagedFile::MakeTop( int n )
{
	if ( n == 0 )
		return;
	Page p = Pages[n];
	for ( int i = n; i > 0; i-- )
		Pages[i] = Pages[i - 1];
	Pages[0] = p;
}

int PagedFile::AcquirePage( uint64 pos )
{
	RetirePage( NPages - 1 ); // error leak
	MakeTop( NPages - 1 );
	Pages[0].Pos = pos;
	size_t bringIn = 0;
	if ( pos < Raw->Length() )
	{
		bringIn = (size_t) min( (uint64) PageSize, (uint64) Raw->Length() - pos );
		Raw->ReadAt( Pages[0].Pos, Pages[0].Buf, bringIn ); // error leak
	}
	// clear remaining space in buffer
	memset( Pages[0].Buf + bringIn, 0, PageSize - bringIn );
	return 0;
}

}