#pragma once

#include "nuDefs.h"
#include "nuTextDefs.h"

/* This stores only font metadata such as filename.

If you're looking for glyphs, they are stored inside nuGlyphCache.

All public members are thread safe.

Members suffixed by "_Internal" assume that the appropriate locks have been acquired.

Once a nuFont* object has been created, it will never be destroyed.

Although the public API of this class is thread safe, the nuFont* objects returned are
most definitely not thread safe. Freetype stores a lot of glyph rendering state inside
the FT_Face object, so only one thread can use a Freetype face at a time.

*/
class NUAPI nuFontStore
{
public:
					nuFontStore();
					~nuFontStore();

	void			Clear();
	void			InitializeFreetype();
	void			ShutdownFreetype();
	const nuFont*	GetByFontID( nuFontID fontID );
	const nuFont*	GetByFacename( const char* facename );
	nuFontID		Insert( const nuFont& font );
	nuFontID		InsertByFacename( const char* facename );

private:
	AbcCriticalSection				Lock;
	pvect<nuFont*>					Fonts;
	fhashmap<nuString, nuFontID>	FacenameToFontID;
	FT_Library						FTLibrary;

	const nuFont*	GetByFacename_Internal( const char* facename ) const;
	nuFontID		Insert_Internal( const nuFont& font );
	const char*		FacenameToFilename( const char* facename );

};
