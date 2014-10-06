#include "pch.h"
#include "xoImageStore.h"
#include "xoImage.h"

const char* xoImageStore::NullImageName		= "NULL";

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
	NextAnon = 1;
}

xoImageStore::~xoImageStore()
{
	delete_all( ImageList );
	NameToIndex.clear();
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
		if ( FreeIndices.size() != 0 )
		{
			index = FreeIndices.rpop();
			ImageList[index] = img;
		}
		else
		{
			index = (int) ImageList.size();
			ImageList += img;
		}

		NameToIndex.insert( sname, index );
	}
}

xoString xoImageStore::SetAnonymous( xoImage* img )
{
	char buf[64] = "!~";
	while ( true )
	{
		xoItoa( NextAnon++, buf + 2, 36 );
		if ( !NameToIndex.contains(xoTempString(buf)) )
		{
			Set( buf, img );
			return buf;
		}
	}
	XOPANIC("xoImageStore failed to generate anonymous name");
	return "";
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

void xoImageStore::Delete( const char* name )
{
	int index = -1;
	if ( NameToIndex.get( xoTempString(name), index ) )
	{
		NameToIndex.erase( xoTempString(name) );
		delete ImageList[index];
		ImageList[index] = nullptr;
		FreeIndices += index;
	}
}

const xoImage* xoImageStore::GetNull() const
{
	return ImageList[NullImageIndex];
}

void xoImageStore::CloneFrom( const xoImageStore& src )
{
	delete_all( ImageList );
	NameToIndex.clear();
	FreeIndices.clear();
	
	Set( NullImageName, src.Get( NullImageName )->Clone() );

	for ( auto it = src.NameToIndex.begin(); it != src.NameToIndex.end(); it++ )
	{
		if ( it.val() != NullImageIndex )
			Set( it.key().Z, src.ImageList[it.val()]->Clone() );
	}
}
