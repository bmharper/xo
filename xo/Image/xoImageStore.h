#pragma once

class xoImage;

// TODO: Add some kind of locking mechanism, so that an imagestore can be shared by a mutating UI thread
// and a rendering thread.
class XOAPI xoImageStore
{
public:
	static const char*	NullImageName;	// You may not use this name "NULL", because it is reserved for the one-and-only null image

					xoImageStore();
					~xoImageStore();

	void			Set( const char* name, xoImage* img );
	xoImage*		Get( const char* name ) const;
	xoImage*		GetOrNull( const char* name ) const;
	const xoImage*	GetNull() const;	// Get the 'null' image, which is a 2x2 checkerboard

	// temp hack. shouldn't be here.
	void			CloneFrom( const xoImageStore& src ); 

protected:
	static const int	NullImageIndex = 0;

	pvect<xoImage*>				ImageList;
	fhashmap<xoString, int>		NameToIndex;

};

