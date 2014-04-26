#include "pch.h"
#include "nuFontStore.h"

nuFontTableImmutable::nuFontTableImmutable()
{
}

nuFontTableImmutable::~nuFontTableImmutable()
{
}

void nuFontTableImmutable::Initialize( const pvect<nuFont*>& fonts )
{
	Fonts = fonts;
}

const nuFont* nuFontTableImmutable::GetByFontID( nuFontID fontID ) const
{
	NUASSERT( fontID != nuFontIDNull && fontID < Fonts.size() );
	return Fonts[fontID];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

const nuFont* nuFontStore::GetByFacename( const char* facename )
{
	TakeCriticalSection lock(Lock);
	return GetByFacename_Internal( facename );
}

nuFontID nuFontStore::Insert( const nuFont& font )
{
	NUASSERT( font.ID == nuFontIDNull );
	NUASSERT( font.Facename.Length() != 0 );
	TakeCriticalSection lock(Lock);
	
	const nuFont* existing = GetByFacename_Internal( font.Facename.Z );
	if ( existing )
		return existing->ID;

	return Insert_Internal( font );
}

nuFontID nuFontStore::InsertByFacename( const char* facename )
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
		NUTRACE( "Failed to load font (facename=%s) (filename=%s)\n", facename, filename );
		return nuFontIDNull;
	}

	return Insert_Internal( font );
}

nuFontID nuFontStore::GetFallbackFontID()
{
	nuFontID fid = nuFontIDNull;
#if NU_PLATFORM_WIN_DESKTOP
	fid = InsertByFacename( "Arial" );
#elif NU_PLATFORM_ANDROID
	fid = InsertByFacename( "Droid Sans" );
#else
	NUTODO_STATIC;
#endif
	NUASSERT( fid != nuFontIDNull );
	return fid;
}

nuFontTableImmutable nuFontStore::GetImmutableTable()
{
	TakeCriticalSection lock(Lock);
	nuFontTableImmutable t;
	t.Initialize( Fonts );
	return t;
}

const nuFont* nuFontStore::GetByFacename_Internal( const char* facename ) const
{
	nuFontID id;
	if ( FacenameToFontID.get( nuTempString(facename), id ) )
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
	LoadFontConstants( *copy );
	return copy->ID;
}

void nuFontStore::LoadFontConstants( nuFont& font )
{
	uint ftflags = FT_LOAD_LINEAR_DESIGN;
	FT_UInt iFTGlyph = FT_Get_Char_Index( font.FTFace, 32 );
	FT_Error e = FT_Set_Pixel_Sizes( font.FTFace, 100, 100 );
	NUASSERT( e == 0 );
	e = FT_Load_Glyph( font.FTFace, iFTGlyph, ftflags );
	if ( e != 0 )
	{
		NUTRACE( "Failed to load glyph for character %d (%d)\n", 32, iFTGlyph );
		font.LinearHoriAdvance_Space_x256 = 0;
	}
	else
	{
		font.LinearHoriAdvance_Space_x256 = ((int32) font.FTFace->glyph->linearHoriAdvance * 256) / font.FTFace->units_per_EM;
	}
	font.NaturalLineHeight_x256 = font.FTFace->height * 256 / font.FTFace->units_per_EM;
}

const char* nuFontStore::FacenameToFilename( const char* facename )
{
	// We need to build a cache here and save it to disk.
	nuTempString name( facename );
#if NU_PLATFORM_WIN_DESKTOP
	if ( name == "Arial" ) return "c:\\Windows\\Fonts\\arial.ttf";
	if ( name == "Times New Roman" ) return "c:\\Windows\\Fonts\\times.ttf";
	if ( name == "Consolas" ) return "c:\\Windows\\Fonts\\consola.ttf";
	if ( name == "Microsoft Sans Serif" ) return "c:\\Windows\\Fonts\\micross.ttf";
	if ( name == "Verdana" ) return "c:\\Windows\\Fonts\\verdana.ttf";
	if ( name == "Tahoma" ) return "c:\\Windows\\Fonts\\tahoma.ttf";
	if ( name == "Segoe UI" ) return "c:\\Windows\\Fonts\\segoeui.ttf";
	return "c:\\Windows\\Fonts\\arial.ttf";
#else
	if ( name == "Droid Sans" ) return "/system/fonts/DroidSans.ttf";
	if ( name == "Droid Sans Mono" ) return "/system/fonts/DroidSansMono.ttf";
	return "/system/fonts/DroidSans.ttf";
#endif
}
