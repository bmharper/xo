#pragma once

class nuImage;

// TODO: Add some kind of locking mechanism, so that an imagestore can be shared by a mutating UI thread
// and a rendering thread.
class NUAPI nuImageStore
{
public:
	static const char*	NullImageName;	// You may not use this name "NULL", because it is reserved for the one-and-only null image

					nuImageStore();
					~nuImageStore();

	void			Set( const char* name, nuImage* img );
	nuImage*		Get( const char* name ) const;
	nuImage*		GetOrNull( const char* name ) const;
	const nuImage*	GetNull() const;	// Get the 'null' image, which is a 2x2 checkerboard

	// temp hack. shouldn't be here.
	void			CloneFrom( const nuImageStore& src ); 

protected:
	static const int	NullImageIndex = 0;

	pvect<nuImage*>				ImageList;
	fhashmap<nuString, int>		NameToIndex;

};

