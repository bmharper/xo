#include "pch.h"
#include "xoImageStore.h"
#include "xoImage.h"

const char* xoImageStore::NullImageName = "NULL";

xoImageStore::xoImageStore()
{
	xoImage* nimg = new xoImage();
	u32 ndata[2][2] = {
		{0xffffffff, 0xff000000},
		{0xff000000, 0xffffffff},
	};
	nimg->Set( xoTexFormatRGBA8, 2, 2, ndata );
	XOASSERT( NullImageIndex == ImageList.size() );
	Set( NullImageName, nimg );
}

xoImageStore::~xoImageStore()
{
	delete_all( ImageList );
}

void xoImageStore::Set( const char* name, xoImage* img )
{
	xoTempString sname(name);
	int index = -1;
	bool exists = NameToIndex.get( sname, index );
	if ( exists )
	{
		delete ImageList[index];
		ImageList[index] = img;
	}
	else
	{
		NameToIndex.insert( sname, (int) ImageList.size() );
		ImageList += img;
	}
}

xoImage* xoImageStore::Get( const char* name ) const
{
	int index = -1;
	if ( NameToIndex.get( xoTempString(name), index ) )
		return ImageList[index];
	else
		return NULL;
}

xoImage* xoImageStore::GetOrNull( const char* name ) const
{
	xoImage* img = Get( name );
	if ( img )
		return img;
	return ImageList[NullImageIndex];
}

const xoImage* xoImageStore::GetNull() const
{
	return ImageList[NullImageIndex];
}

void xoImageStore::CloneFrom( const xoImageStore& src )
{
	delete_all( ImageList );
	NameToIndex.clear();
	
	Set( NullImageName, src.Get( NullImageName )->Clone() );

	for ( auto it = src.NameToIndex.begin(); it != src.NameToIndex.end(); it++ )
	{
		if ( it.val() != NullImageIndex )
			Set( it.key().Z, src.ImageList[it.val()]->Clone() );
	}
}
