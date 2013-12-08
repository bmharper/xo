#include "pch.h"
#include "nuImageStore.h"
#include "nuImage.h"

const char* nuImageStore::NullImageName = "NULL";

nuImageStore::nuImageStore()
{
	nuImage* nimg = new nuImage();
	u32 ndata[2][2] = {
		{0xffffffff, 0xff000000},
		{0xff000000, 0xffffffff},
	};
	nimg->Set( nuTexFormatRGBA8, 2, 2, ndata );
	NUASSERT( NullImageIndex == ImageList.size() );
	Set( NullImageName, nimg );
}

nuImageStore::~nuImageStore()
{
	delete_all( ImageList );
}

void nuImageStore::Set( const char* name, nuImage* img )
{
	nuTempString sname(name);
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

nuImage* nuImageStore::Get( const char* name ) const
{
	int index = -1;
	if ( NameToIndex.get( nuTempString(name), index ) )
		return ImageList[index];
	else
		return NULL;
}

nuImage* nuImageStore::GetOrNull( const char* name ) const
{
	nuImage* img = Get( name );
	if ( img )
		return img;
	return ImageList[NullImageIndex];
}

const nuImage* nuImageStore::GetNull() const
{
	return ImageList[NullImageIndex];
}

void nuImageStore::CloneFrom( const nuImageStore& src )
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
