#include "pch.h"
#include "xoFontStore.h"

static const int ManifestVersion = 1;
static const char UnitSeparator = 31;

xoFontTableImmutable::xoFontTableImmutable()
{
}

xoFontTableImmutable::~xoFontTableImmutable()
{
}

void xoFontTableImmutable::Initialize(const pvect<xoFont*>& fonts)
{
	Fonts = fonts;
}

const xoFont* xoFontTableImmutable::GetByFontID(xoFontID fontID) const
{
	XOASSERT(fontID != xoFontIDNull && fontID < Fonts.size());
	return Fonts[fontID];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xoFontStore::xoFontStore()
{
	FTLibrary = NULL;
	Fonts += NULL;
	XOASSERT(Fonts[xoFontIDNull] == NULL);
	IsFontTableLoaded = false;
	AbcCriticalSectionInitialize(Lock);

#if XO_PLATFORM_WIN_DESKTOP
	wchar_t* wpath;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &wpath)))
	{
		Directories += ConvertWideToUTF8(wpath).c_str();
		CoTaskMemFree(wpath);
	}
#elif XO_PLATFORM_LINUX_DESKTOP
	Directories += "/usr/share/fonts/truetype";
	Directories += "/usr/share/fonts/truetype/msttcorefonts";
#elif XO_PLATFORM_ANDROID
	Directories += "/system/fonts";
#else
	XOTODO_STATIC
#endif
}

xoFontStore::~xoFontStore()
{
	AbcCriticalSectionDestroy(Lock);
}

void xoFontStore::Clear()
{
	TakeCriticalSection lock(Lock);
	for (intp i = xoFontIDNull + 1; i < Fonts.size(); i++)
	{
		FT_Done_Face(Fonts[i]->FTFace);
		Fonts[i]->FTFace = NULL;
	}
	delete_all(Fonts);
	FacenameToFontID.clear();
}

void xoFontStore::InitializeFreetype()
{
	FT_Init_FreeType(&FTLibrary);
}

void xoFontStore::ShutdownFreetype()
{
	FT_Done_FreeType(FTLibrary);
	FTLibrary = NULL;
}

const xoFont* xoFontStore::GetByFontID(xoFontID fontID)
{
	XOASSERT(fontID != xoFontIDNull && fontID < Fonts.size());
	TakeCriticalSection lock(Lock);
	return Fonts[fontID];
}

const xoFont* xoFontStore::GetByFacename(const char* facename)
{
	TakeCriticalSection lock(Lock);
	return GetByFacename_Internal(facename);
}

xoFontID xoFontStore::Insert(const xoFont& font)
{
	XOASSERT(font.ID == xoFontIDNull);
	XOASSERT(font.Facename.Length() != 0);
	TakeCriticalSection lock(Lock);

	const xoFont* existing = GetByFacename_Internal(font.Facename.Z);
	if (existing)
		return existing->ID;

	return Insert_Internal(font);
}

xoFontID xoFontStore::InsertByFacename(const char* facename)
{
	TakeCriticalSection lock(Lock);

	const xoFont* existing = GetByFacename_Internal(facename);
	if (existing)
		return existing->ID;

	xoFont font;
	font.Facename = facename;
	const char* filename = GetFilenameFromFacename(facename);

	if (filename == nullptr)
	{
		XOTRACE("Failed to load font (facename=%s) (font not found)\n", facename);
		return xoFontIDNull;
	}

	FT_Error e = FT_New_Face(FTLibrary, filename, 0, &font.FTFace);
	if (e != 0)
	{
		XOTRACE("Failed to load font (facename=%s) (filename=%s)\n", facename, filename);
		return xoFontIDNull;
	}

	return Insert_Internal(font);
}

xoFontID xoFontStore::GetFallbackFontID()
{
	xoFontID fid = xoFontIDNull;
#if XO_PLATFORM_WIN_DESKTOP
	fid = InsertByFacename("Arial");
#elif XO_PLATFORM_ANDROID
	fid = InsertByFacename("Roboto");
#elif XO_PLATFORM_LINUX_DESKTOP
	fid = InsertByFacename("Arial");
#else
	XOTODO_STATIC;
#endif
	XOASSERT(fid != xoFontIDNull);
	return fid;
}

xoFontTableImmutable xoFontStore::GetImmutableTable()
{
	TakeCriticalSection lock(Lock);
	xoFontTableImmutable t;
	t.Initialize(Fonts);
	return t;
}

void xoFontStore::AddFontDirectory(const char* dir)
{
	TakeCriticalSection lock(Lock);
	Directories += dir;
	IsFontTableLoaded = false;
}

const xoFont* xoFontStore::GetByFacename_Internal(const char* facename) const
{
	xoFontID id;
	if (FacenameToFontID.get(xoTempString(facename), id))
		return Fonts[id];
	else
		return nullptr;
}

xoFontID xoFontStore::Insert_Internal(const xoFont& font)
{
	xoFont* copy = new xoFont();
	*copy = font;
	copy->ID = (xoFontID) Fonts.size();
	Fonts += copy;
	FacenameToFontID.insert(copy->Facename, copy->ID);
	LoadFontConstants(*copy);
	LoadFontTweaks(*copy);
	return copy->ID;
}

#define EM_TO_256(x) ((int32) ((x) * 256 / font.FTFace->units_per_EM))

void xoFontStore::LoadFontConstants(xoFont& font)
{
	uint ftflags = FT_LOAD_LINEAR_DESIGN;
	FT_UInt iFTGlyph = FT_Get_Char_Index(font.FTFace, 32);
	FT_Error e = FT_Set_Pixel_Sizes(font.FTFace, 100, 100);
	XOASSERT(e == 0);
	e = FT_Load_Glyph(font.FTFace, iFTGlyph, ftflags);
	if (e != 0)
	{
		XOTRACE("Failed to load glyph for character %d (%d)\n", 32, iFTGlyph);
		font.LinearHoriAdvance_Space_x256 = 0;
	}
	else
	{
		font.LinearHoriAdvance_Space_x256 = EM_TO_256(font.FTFace->glyph->linearHoriAdvance);
	}
	font.LineHeight_x256 = EM_TO_256(font.FTFace->height);
	font.Ascender_x256 = EM_TO_256(font.FTFace->ascender);
	font.Descender_x256 = EM_TO_256(font.FTFace->descender);
}

void xoFontStore::LoadFontTweaks(xoFont& font)
{
	// The default of 15 is a number taken from observations of a bunch of different fonts
	// The auto hinter fails to produce clean horizontal stems when the text gets larger
	// Times New Roman seems to look better at all sub-pixel sizes using the auto hinter.
	// The Sans Serif fonts like Segoe UI seem to look better with the TT hinter at larger sizes.

	if (font.Facename == "Times New Roman")
		font.MaxAutoHinterSize = 30;

	if (font.Facename == "Microsoft Sans Serif")
		font.MaxAutoHinterSize = 14;

	// This always looks better with the TT hinter
	if (font.Facename == "Segoe UI")
		font.MaxAutoHinterSize = 0;

	// This always looks better with the TT hinter
	//if ( font.Facename == "Audiowide" )
	//	font.MaxAutoHinterSize = 50;
}

const char* xoFontStore::GetFilenameFromFacename(const char* facename)
{
	if (!IsFontTableLoaded)
	{
		if (!LoadFontTable())
			BuildAndSaveFontTable();
	}

	xoString name = facename;
	xoString* fn = FacenameToFilename.getp(name);
	if (fn != nullptr)
		return fn->Z;

	name += " Regular";
	fn = FacenameToFilename.getp(name);
	if (fn != nullptr)
		return fn->Z;

	return nullptr;

	/*
	// We need to build a cache here and save it to disk.
	xoTempString name( facename );
	#if XO_PLATFORM_WIN_DESKTOP
	if ( name == "Arial" ) return "c:\\Windows\\Fonts\\arial.ttf";
	if ( name == "Times New Roman" ) return "c:\\Windows\\Fonts\\times.ttf";
	if ( name == "Consolas" ) return "c:\\Windows\\Fonts\\consola.ttf";
	if ( name == "Microsoft Sans Serif" ) return "c:\\Windows\\Fonts\\micross.ttf";
	if ( name == "Verdana" ) return "c:\\Windows\\Fonts\\verdana.ttf";
	if ( name == "Tahoma" ) return "c:\\Windows\\Fonts\\tahoma.ttf";
	if ( name == "Segoe UI" ) return "c:\\Windows\\Fonts\\segoeui.ttf";
	return "c:\\Windows\\Fonts\\arial.ttf";
	#elif XO_PLATFORM_LINUX_DESKTOP
	if ( name == "Arial" ) return "/usr/share/fonts/truetype/msttcorefonts/arial.ttf";
	if ( name == "Times New Roman" ) return "/usr/share/fonts/truetype/msttcorefonts/times.ttf";
	return "/usr/share/fonts/truetype/msttcorefonts/arial.ttf";
	#else
	if ( name == "Droid Sans" ) return "/system/fonts/DroidSans.ttf";
	if ( name == "Droid Sans Mono" ) return "/system/fonts/DroidSansMono.ttf";
	return "/system/fonts/DroidSans.ttf";
	#endif
	*/
}

void xoFontStore::BuildAndSaveFontTable()
{
	XOTRACE("Building font table on %d directories\n", (int) Directories.size());

	podvec<xoString> files;
	for (intp i = 0; i < Directories.size(); i++)
	{
		auto cb = [&](const AbcFilesystemItem& item) -> bool
		{
			if (IsFontFilename(item.Name))
				files += xoString(item.Root) + ABC_DIR_SEP_STR + item.Name;
			return true;
		};

		AbcFilesystemFindFiles(Directories[i].Z, cb);
	}

	xoString cacheFile = xoGlobal()->CacheDir + ABC_DIR_SEP_STR + "fonts";
	FILE* manifest = fopen(cacheFile.Z, "wb");
	if (manifest == nullptr)
	{
		XOTRACE("Failed to open font cache file %s. Aborting.\n", cacheFile.Z);
		XOPANIC("Failed to open font cache file");
	}
	fprintf(manifest, "%d\n", ManifestVersion);
	fprintf(manifest, "%llu\n", (long long unsigned) ComputeFontDirHash());

	for (intp i = 0; i < files.size(); i++)
	{
		FT_Face face;
		FT_Error e = FT_New_Face(FTLibrary, files[i].Z, 0, &face);
		if (e == 0)
		{
			xoString facename = face->family_name;
			xoString style = face->style_name;
			xoString fullFacename = facename + " " + style;
			fprintf(manifest, "%s%c%s\n", files[i].Z, UnitSeparator, fullFacename.Z);
			FT_Done_Face(face);
		}
		else
		{
			XOTRACE("Failed to load font (filename=%s)\n", files[i].Z);
		}
	}

	fclose(manifest);

	LoadFontTable();
}

bool xoFontStore::LoadFontTable()
{
	XOTRACE_FONTS("LoadFontTable enter\n");

	FacenameToFilename.clear();
	FILE* manifest = fopen((xoGlobal()->CacheDir + ABC_DIR_SEP_STR + "fonts").Z, "rb");
	if (manifest == nullptr)
		return false;

	XOTRACE_FONTS("LoadFontTable file loaded\n");

	int version = 0;
	uint64 hash = 0;
	bool ok = true;
	ok = ok && 1 == fscanf(manifest, "%d\n", &version);
	ok = ok && 1 == fscanf(manifest, "%llu\n", (long long unsigned*) &hash);
	if (version != ManifestVersion || hash != ComputeFontDirHash())
	{
		fclose(manifest);
		return false;
	}

	XOTRACE_FONTS("LoadFontTable version & hash good\n");

	// Read the rest of the file into one buffer
	podvec<char> remain;
	{
		char buf[1024];
		size_t nbytes = 0;
		while ((nbytes = fread(buf, 1, sizeof(buf), manifest)) != 0)
			remain.addn(buf, nbytes);
	}

	fclose(manifest);

	// Parse the contents out line by line, using character 31 as a field separator
	const char* buf = &remain[0];
	intp lineStart = 0;
	intp term1 = 0;
	for (intp i = 0; true; i++)
	{
		if (i == remain.size() || buf[i] == '\n')
		{
			if (i - term1 > 1)
			{
				xoString path, facename;
				path.Set(buf + lineStart, term1 - lineStart);
				facename.Set(buf + term1 + 1, i - term1 - 1);
				FacenameToFilename.insert(facename, path);
				XOTRACE_FONTS("Font %s -> %s\n", facename.Z, path.Z);
			}
			lineStart = i + 1;
			term1 = lineStart;
		}
		if (i == remain.size())
			break;
		if (buf[i] == UnitSeparator)
			term1 = i;
	}

	IsFontTableLoaded = true;

	XOTRACE_FONTS("LoadFontTable success (%d fonts)\n", FacenameToFilename.size());

	return true;
}

uint64 xoFontStore::ComputeFontDirHash()
{
	void* hstate = XXH64_init(0);

	auto cb = [&](const AbcFilesystemItem& item) -> bool
	{
		if (IsFontFilename(item.Name))
		{
			XXH64_update(hstate, item.Root, (int) strlen(item.Root));
			XXH64_update(hstate, item.Name, (int) strlen(item.Name));
			XXH64_update(hstate, &item.TimeModify, sizeof(item.TimeModify));
		}
		return true;
	};

	for (intp i = 0; i < Directories.size(); i++)
		AbcFilesystemFindFiles(Directories[i].Z, cb);

	return (uint64) XXH64_digest(hstate);
}

bool xoFontStore::IsFontFilename(const char* filename)
{
	return	strstr(filename, ".ttf") != nullptr ||
			strstr(filename, ".ttc ") != nullptr;
}
