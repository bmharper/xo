#pragma once

class xoImage;

// TODO: Add some kind of locking mechanism, so that an imagestore can be shared by a mutating UI thread
// and a rendering thread.
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

	// temp hack. shouldn't be here. We should be doing some kind of copy-on-write functionality.
	// Probably the UI thread does the "copy" when it wants to mutate an image that has been locked
	// by the renderer. Alternatively, the renderer locks the image briefly before it uploads it
	// to the GPU.
	void				CloneFrom( const xoImageStore& src ); 

protected:
	static const int	NullImageIndex = 0;

	pvect<xoImage*>				ImageList;
	fhashmap<xoString, int>		NameToIndex;
	podvec<int>					FreeIndices;
	int64						NextAnon;
};

