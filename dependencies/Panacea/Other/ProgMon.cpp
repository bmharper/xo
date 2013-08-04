#include "pch.h"
#include "ProgMon.h"

namespace AbCore
{

ProgThreadMarshall::ProgThreadMarshall()
{
	IsCancelled = false;
	IsCancelledAllowed = true;
	Sink = NULL;
	AbcCriticalSectionInitialize( Lock, 4000 );
}

ProgThreadMarshall::~ProgThreadMarshall()
{
	for ( intp i = 0; i < Chunks.size(); i++ )
		free( Chunks[i].Data );
	AbcCriticalSectionDestroy( Lock );
}

void ProgThreadMarshall::ProgSet( XString topic, double percent )
{
	AbcCriticalSectionEnter( Lock );
	Item& it = Queue.add();
	const wchar_t* str;
	CopyStr( topic, str );
	it.Topic = str;
	it.Percent = percent;
	AbcCriticalSectionLeave( Lock );
}

bool ProgThreadMarshall::ProgIsCancelled()
{
	return !!IsCancelled;
}

bool ProgThreadMarshall::ProgAllowCancel( bool allow )
{
	AbcCriticalSectionEnter( Lock );
	uint32 p = IsCancelledAllowed;		// effectively lose previous state here
	IsCancelledAllowed = allow;
	AbcCriticalSectionLeave( Lock );
	return !!p;
}

void ProgThreadMarshall::Attach( IProgMon* sink, bool allowCancel )
{
	Sink = sink;
	IsCancelled = sink->ProgIsCancelled();
	IsCancelledAllowed = allowCancel;
}

void ProgThreadMarshall::Poll()
{
	AbcCriticalSectionEnter( Lock );

	Sink->ProgAllowCancel( !!IsCancelledAllowed );	// we lose the previous state here.
	IsCancelled = Sink->ProgIsCancelled();

	for ( intp i = 0; i < Queue.size(); i++ )
		Sink->ProgSet( Queue[i].Topic, Queue[i].Percent );

	Queue.clear_noalloc();

	for ( intp i = 0; i < Chunks.size(); i++ )
		Chunks[i].Pos = 0;

	AbcCriticalSectionLeave( Lock );
}

void ProgThreadMarshall::CopyStr( const XString& src, const wchar_t*& s )
{
	if ( src.Length() == 0 )
	{
		s = L"";
		return;
	}

	size_t nchar = src.Length() + 1;

	// Check if the last chunk has space for us
	intp ichunk = -1;
	if (	(Chunks.size() > 0) &&
			(Chunks.back().Size - Chunks.back().Pos >= nchar) )
	{
		ichunk = Chunks.size() - 1;
	}

	if ( ichunk == -1 )
	{
		auto& c = Chunks.add();
		c.Pos = 0;
		c.Size = std::max((size_t) 8192, nchar);
		c.Data = (wchar_t*) malloc( sizeof(wchar_t) * c.Size );
		ichunk = Chunks.size() - 1;
	}

	auto& c = Chunks[ichunk];
	s = &c.Data[c.Pos];
	wcscpy( &c.Data[c.Pos], src );
	c.Pos += nchar;
}


}
