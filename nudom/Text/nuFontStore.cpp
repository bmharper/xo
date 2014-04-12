#include "pch.h"
#include "nuFontStore.h"

nuFontStore::nuFontStore()
{
	FTLibrary = NULL;
	Fonts += NULL;
	NUASSERT( Fonts[nuFontIDNull] == NULL );
	AbcCriticalSectionInitialize( Lock );
}

nuFontStore::~nuFontStore()
{
	AbcCriticalSectionDestroy( Lock );
}

void nuFontStore::Clear()
{
	TakeCriticalSection lock(Lock);
	for ( intp i = nuFontIDNull + 1; i < Fonts.size(); i++ ) 
	{
		FT_Done_Face( Fonts[i]->FTFace );
		Fonts[i]->FTFace = NULL;
	}
	delete_all( Fonts );
	FacenameToFontID.clear();
}

void nuFontStore::InitializeFreetype()
{
	FT_Init_FreeType( &FTLibrary );
}

void nuFontStore::ShutdownFreetype()
{
	FT_Done_FreeType( FTLibrary );
	FTLibrary = NULL;
}

const nuFont* nuFontStore::GetByFontID( nuFontID fontID )
{
	NUASSERT( fontID != nuFontIDNull && fontID < Fonts.size() );
	TakeCriticalSection lock(Lock);
	return Fonts[fontID];
}

const nuFont* nuFontStore::GetByFacename( const nuString& facename )
{
	TakeCriticalSection lock(Lock);
	return GetByFacename_Internal( facename );
}

nuFontID nuFontStore::Insert( const nuFont& font )
{
	NUASSERT( font.ID == nuFontIDNull );
	NUASSERT( font.Facename.Length() != 0 );
	TakeCriticalSection lock(Lock);
	
	const nuFont* existing = GetByFacename_Internal( font.Facename );
	if ( existing )
		return existing->ID;

	return Insert_Internal( font );
}

nuFontID nuFontStore::InsertByFacename( const nuString& facename )
{
	TakeCriticalSection lock(Lock);

	const nuFont* existing = GetByFacename_Internal( facename );
	if ( existing )
		return existing->ID;

	nuFont font;
	font.Facename = facename;
	const char* filename = FacenameToFilename( facename );

	FT_Error e = FT_New_Face( FTLibrary, filename, 0, &font.FTFace );
	if ( e != 0 )
	{
		NUTRACE( "Failed to load font (facename=%s) (filename=%s)\n", facename.Z, filename );
		return nuFontIDNull;
	}

	return Insert_Internal( font );
}

const nuFont* nuFontStore::GetByFacename_Internal( const nuString& facename ) const
{
	nuFontID id;
	if ( FacenameToFontID.get( facename, id ) )
		return Fonts[id];
	else
		return NULL;
}

nuFontID nuFontStore::Insert_Internal( const nuFont& font )
{
	nuFont* copy = new nuFont();
	*copy = font;
	copy->ID = (nuFontID) Fonts.size();
	Fonts += copy;
	FacenameToFontID.insert( copy->Facename, copy->ID );
	return copy->ID;
}

const char* nuFontStore::FacenameToFilename( const nuString& facename )
{
#if NU_PLATFORM_WIN_DESKTOP
	if ( facename == "Arial" ) return "c:\\Windows\\Fonts\\arial.ttf";
	if ( facename == "Times New Roman" ) return "c:\\Windows\\Fonts\\times.ttf";
	if ( facename == "Consolas" ) return "c:\\Windows\\Fonts\\consola.ttf";
	if ( facename == "Microsoft Sans Serif" ) return "c:\\Windows\\Fonts\\micross.ttf";
	if ( facename == "Verdana" ) return "c:\\Windows\\Fonts\\verdana.ttf";
	if ( facename == "Tahoma" ) return "c:\\Windows\\Fonts\\tahoma.ttf";
	return "c:\\Windows\\Fonts\\arial.ttf";
#else
	if ( facename == "Droid Sans" ) return "/system/fonts/DroidSans.ttf";
	if ( facename == "Droid Sans Mono" ) return "/system/fonts/DroidSansMono.ttf";
	return "/system/fonts/DroidSans.ttf";
#endif
}
