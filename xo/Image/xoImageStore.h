#pragma once

class xoImage;

// Store a set of named images.
// TODO: Improve performance of CloneMetadataFrom(). It always blows away the entire table and recreates it from scratch.
class XOAPI xoImageStore
{
public:
	static const char*	NullImageName;			// You may not use this name "NULL", because it is reserved for the one-and-only null image

						xoImageStore();
						~xoImageStore();

	void				Set( const char* name, xoImage* img );
	xoString			SetAnonymous( xoImage* img );				// Creates a unique name for the image, inserts the image, and returns the name
	xoImage*			Get( const char* name ) const;
	xoImage*			GetOrNull( const char* name ) const;
	const xoImage*		GetNull() const;							// Get the 'null' image, which is a 2x2 checkerboard
	void				Delete( const char* name );
	pvect<xoImage*>		InvalidList() const;

	void				CloneMetadataFrom( const xoImageStore& src ); 

protected:
	static const int	NullImageIndex = 0;

	pvect<xoImage*>				ImageList;
	fhashmap<xoString, int>		NameToIndex;
	podvec<int>					FreeIndices;
	int64						NextAnon;
};

