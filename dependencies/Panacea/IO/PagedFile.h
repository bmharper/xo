#pragma once

#include "IFile.h"

namespace AbCore
{

/** IFile wrapper that only reads/writes on page boundaries. 
I created this after creating Panacea::IO::BufferedFile. BufferedFile is already quite complex, and is used in lots of places.
So rather than mess with it, I just decided to recreate something for this purpose. I needed this thing in order to
be able to open a file with all caching turned off. This was for the sake of Mip2, which will run through jobs of
many gigabytes in a streaming fashion.

Design:
We have a fixed number of cached pages (NPages = 4), all of the same size, which must be a power of 2.
If you attempt to read or write from a page that is not in the cache, then it is first brought in. Pages are zero-filled
where not specified by the file system behaviour.
Whenever a page is used, it is swapped to position 0, with other pages bumping down. When we need to retire a page,
we always retire the page at the end of the buffer chain. This is a very simple LRU scheme.

[2013-07-31 BMH] I have no idea what I was thinking when I wrote this. It's VERY similar to BufferedFile. However,
it is far simpler, and it is used by Mip2, so instead of messing with that, I rather leave this as is. This thing
is very simple - a plain old user-space buffered file.
**/
class PAPI PagedFile : public AbCore::IFile
{
public:
				PagedFile();
	virtual		~PagedFile();
	
	void		Reset();
	void		Init( AbCore::IFile* raw, uint pageBits );

	void		Sleep();

	IFile*		GetRaw() { return Raw; }

	virtual bool		Error();
	virtual XString		ErrorMessage();
	virtual void		ClearError();

	virtual bool		Flush( int flags );

	virtual size_t		Write( const void* data, size_t bytes );
	virtual size_t		Read( void* data, size_t bytes );

	virtual bool		SetLength( UINT64 len );
	virtual UINT64		Position();
	virtual UINT64		Length();
	virtual bool		Seek( UINT64 pos );

protected:
	struct Page
	{
		uint64	Pos;
		bool	Dirty;
		byte*	Buf;
		void Reset() { Pos = -1; Dirty = false; Buf = NULL; }
	};
	static const int NPages = 4;

	AbCore::IFile*	Raw;
	UINT64			Pos;
	UINT64			CachedLength;
	uint64			PageMask;
	uint			PageBits;
	uint			PageSize;
	Page			Pages[NPages];
	void*			WholeBuf;

	void			Wake();
	int				GetPage( uint64 pos );
	bool			RetirePage( int n );
	void			MakeTop( int n );
	int				AcquirePage( uint64 pos );
	
	template<bool TWrite>
	size_t			ReadOrWrite( void* data, size_t bytes );
};

}
