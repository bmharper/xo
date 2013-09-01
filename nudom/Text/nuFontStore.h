#pragma once

#include "nuDefs.h"
#include "nuTextDefs.h"

/* This stores only font metadata such as filename.

If you're looking for glyphs, they are stored inside nuGlyphCache.

All public members are thread safe.

Members suffixed by "Internal" assume that the appropriate locks have been acquired.

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
	const nuFont*	GetByFacename( const nuString& facename );
	nuFontID		Insert( const nuFont& font );
	nuFontID		InsertByFacename( const nuString& facename );

private:
	AbcCriticalSection				Lock;
	pvect<nuFont*>					Fonts;
	fhashmap<nuString, nuFontID>	FacenameToFontID;
	FT_Library						FTLibrary;

	const nuFont*	GetByFacenameInternal( const nuString& facename ) const;
	nuFontID		InsertInternal( const nuFont& font );
	const char*		FacenameToFilename( const nuString& facename );

};
