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
	return GetByFacenameInternal( facename );
}

nuFontID nuFontStore::Insert( const nuFont& font )
{
	NUASSERT( font.ID == nuFontIDNull );
	NUASSERT( font.Facename.Len != 0 );
	TakeCriticalSection lock(Lock);
	
	const nuFont* existing = GetByFacenameInternal( font.Facename );
	if ( existing )
		return existing->ID;

	return InsertInternal( font );
}

nuFontID nuFontStore::InsertByFacename( const nuString& facename )
{
	TakeCriticalSection lock(Lock);

	const nuFont* existing = GetByFacenameInternal( facename );
	if ( existing )
		return existing->ID;

	nuFont font;
	font.Facename = facename;
	const char* filename = "c:\\Windows\\Fonts\\arial.ttf";

	FT_Error e = FT_New_Face( FTLibrary, filename, 0, &font.FTFace );
	if ( e != 0 )
	{
		NUTRACE( "Failed to load font (facename=%s) (filename=%s)\n", facename, filename );
		return nuFontIDNull;
	}

	return InsertInternal( font );
}

const nuFont* nuFontStore::GetByFacenameInternal( const nuString& facename ) const
{
	nuFontID id;
	if ( FacenameToFontID.get( facename, id ) )
		return Fonts[id];
	else
		return NULL;
}

nuFontID nuFontStore::InsertInternal( const nuFont& font )
{
	nuFont* copy = new nuFont();
	*copy = font;
	copy->ID = (nuFontID) Fonts.size();
	Fonts += copy;
	FacenameToFontID.insert( copy->Facename, copy->ID );
	return copy->ID;
}
