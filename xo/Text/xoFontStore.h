#pragma once

#include "../xoDefs.h"
#include "xoTextDefs.h"

// This is used during layout to get an immutable set of fonts that we can access
// without having to take any locks. Once a xoFont object has been created, it is
// never mutated. the Freetype internals are most definitely mutated as we generate
// more glyphs, but the info directly stored inside xoFont is immutable.
// Of particular importance is LinearHoriAdvance_Space_x256, which is used a lot
// during layout.
class XOAPI xoFontTableImmutable
{
public:
					xoFontTableImmutable();
					~xoFontTableImmutable();

	void			Initialize( const pvect<xoFont*>& fonts );
	const xoFont*	GetByFontID( xoFontID fontID ) const;

protected:
	pvect<xoFont*>	Fonts;
};

/* This stores only font metadata such as filename.

If you're looking for glyphs, they are stored inside xoGlyphCache.

All public members are thread safe.

Members suffixed by "_Internal" assume that the appropriate locks have been acquired.

Once a xoFont* object has been created, it will never be destroyed.
Also, since a xoFont object is immutable once created, we can return
a xoFontStoreTable object and know that the objects inside it are
safe to access from many threads, while we still go ahead on the main
thread and create more fonts.

Although the public API of this class is thread safe, the xoFont* objects returned are
most definitely not thread safe. Freetype stores a lot of glyph rendering state inside
the FT_Face object, so only one thread can use a Freetype face at a time.

TODO: Change the font cache file so that the filename includes the hash. At present,
if multiple xo applications run on the same machine, with different sets of font
directories, then they will thrash the font cache file.

*/
class XOAPI xoFontStore
{
public:
							xoFontStore();
							~xoFontStore();

	void					Clear();
	void					InitializeFreetype();
	void					ShutdownFreetype();
	const xoFont*			GetByFontID( xoFontID fontID );
	const xoFont*			GetByFacename( const char* facename );
	xoFontID				Insert( const xoFont& font );
	xoFontID				InsertByFacename( const char* facename );		// This is safe to call if the font is already loaded
	xoFontID				GetFallbackFontID();							// This is a font that is always available on this platform. Panics if the font is not available.
	xoFontTableImmutable	GetImmutableTable();

	void					AddFontDirectory( const char* dir );

private:
	AbcCriticalSection				Lock;
	pvect<xoFont*>					Fonts;
	podvec<xoString>				Directories;
	fhashmap<xoString, xoFontID>	FacenameToFontID;
	fhashmap<xoString, xoString>	FacenameToFilename;
	FT_Library						FTLibrary;
	bool							IsFontTableLoaded;

	const xoFont*	GetByFacename_Internal( const char* facename ) const;
	xoFontID		Insert_Internal( const xoFont& font );
	void			LoadFontConstants( xoFont& font );
	void			LoadFontTweaks( xoFont& font );
	const char*		GetFilenameFromFacename( const char* facename );
	void			BuildAndSaveFontTable();
	bool			LoadFontTable();
	uint64			ComputeFontDirHash();
	
	static bool		IsFontFilename( const char* filename );

};
