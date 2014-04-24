#pragma once

#include "nuDefs.h"
#include "nuTextDefs.h"

// This is used during layout to get an immutable set of fonts that we can access
// without having to take any locks. Once a nuFont object has been created, it is
// never mutated. the Freetype internals are most definitely mutated as we generate
// more glyphs, but the info directly stored inside nuFont is immutable.
// Of particular importance is LinearHoriAdvance_Space_x256, which is used a lot
// during layout.
class NUAPI nuFontTableImmutable
{
public:
					nuFontTableImmutable();
					~nuFontTableImmutable();

	void			Initialize( const pvect<nuFont*>& fonts );
	const nuFont*	GetByFontID( nuFontID fontID ) const;

protected:
	pvect<nuFont*>	Fonts;
};

/* This stores only font metadata such as filename.

If you're looking for glyphs, they are stored inside nuGlyphCache.

All public members are thread safe.

Members suffixed by "_Internal" assume that the appropriate locks have been acquired.

Once a nuFont* object has been created, it will never be destroyed.
Also, since a nuFont object is immutable once created, we can return
a nuFontStoreTable object and know that the objects inside it are
safe to access from many threads, while we still go ahead on the main
thread and create more fonts.

Although the public API of this class is thread safe, the nuFont* objects returned are
most definitely not thread safe. Freetype stores a lot of glyph rendering state inside
the FT_Face object, so only one thread can use a Freetype face at a time.

*/
class NUAPI nuFontStore
{
public:
							nuFontStore();
							~nuFontStore();

	void					Clear();
	void					InitializeFreetype();
	void					ShutdownFreetype();
	const nuFont*			GetByFontID( nuFontID fontID );
	const nuFont*			GetByFacename( const char* facename );
	nuFontID				Insert( const nuFont& font );
	nuFontID				InsertByFacename( const char* facename );		// This is safe to call if the font is already loaded
	nuFontID				GetFallbackFontID();							// This is a font that is always available on this platform. Panics if the font is not available.
	nuFontTableImmutable	GetImmutableTable();

private:
	AbcCriticalSection				Lock;
	pvect<nuFont*>					Fonts;
	fhashmap<nuString, nuFontID>	FacenameToFontID;
	FT_Library						FTLibrary;

	const nuFont*	GetByFacename_Internal( const char* facename ) const;
	nuFontID		Insert_Internal( const nuFont& font );
	void			LoadFontConstants( nuFont& font );
	const char*		FacenameToFilename( const char* facename );

};
