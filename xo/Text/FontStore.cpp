#include "pch.h"
#include "FontStore.h"

namespace xo {

static const int  ManifestVersion = 1;
static const char UnitSeparator   = 31;

FontTableImmutable::FontTableImmutable() {
}

FontTableImmutable::~FontTableImmutable() {
}

void FontTableImmutable::Initialize(const cheapvec<Font*>& fonts) {
	Fonts = fonts;
}

const Font* FontTableImmutable::GetByFontID(FontID fontID) const {
	XO_ASSERT(fontID != FontIDNull && fontID < Fonts.size());
	return Fonts[fontID];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FontStore::FontStore() {
	FTLibrary = NULL;
	Fonts += NULL;
	XO_ASSERT(Fonts[FontIDNull] == NULL);
	IsFontTableLoaded = false;

#if XO_PLATFORM_WIN_DESKTOP
	wchar_t* wpath;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &wpath))) {
		Directories += ConvertWideToUTF8(wpath).c_str();
		CoTaskMemFree(wpath);
	}
#elif XO_PLATFORM_LINUX_DESKTOP
	Directories += "/usr/share/fonts/truetype";
	Directories += "/usr/share/fonts/truetype/msttcorefonts";
#elif XO_PLATFORM_ANDROID
	Directories += "/system/fonts";
#else
	XO_TODO_STATIC
#endif
}

FontStore::~FontStore() {
}

void FontStore::Clear() {
	std::lock_guard<std::mutex> lock(Lock);
	for (size_t i = FontIDNull + 1; i < Fonts.size(); i++) {
		FT_Done_Face(Fonts[i]->FTFace);
		Fonts[i]->FTFace = NULL;
	}
	DeleteAll(Fonts);
	FacenameToFontID.clear();
}

void FontStore::InitializeFreetype() {
	FT_Init_FreeType(&FTLibrary);
}

void FontStore::ShutdownFreetype() {
	FT_Done_FreeType(FTLibrary);
	FTLibrary = NULL;
}

const Font* FontStore::GetByFontID(FontID fontID) {
	XO_ASSERT(fontID != FontIDNull && fontID < Fonts.size());
	std::lock_guard<std::mutex> lock(Lock);
	return Fonts[fontID];
}

const Font* FontStore::GetByFacename(const char* facename) {
	std::lock_guard<std::mutex> lock(Lock);
	return GetByFacename_Internal(facename);
}

FontID FontStore::Insert(const Font& font) {
	XO_ASSERT(font.ID == FontIDNull);
	XO_ASSERT(font.Facename.Length() != 0);
	std::lock_guard<std::mutex> lock(Lock);

	const Font* existing = GetByFacename_Internal(font.Facename.Z);
	if (existing)
		return existing->ID;

	return Insert_Internal(font);
}

FontID FontStore::InsertByFacename(const char* facename) {
	std::lock_guard<std::mutex> lock(Lock);

	const Font* existing = GetByFacename_Internal(facename);
	if (existing)
		return existing->ID;

	Font font;
	font.Facename        = facename;
	const char* filename = GetFilenameFromFacename(facename);

	if (filename == nullptr) {
		Trace("Failed to load font (facename=%s) (font not found)\n", facename);
		return FontIDNull;
	}

	FT_Error e = FT_New_Face(FTLibrary, filename, 0, &font.FTFace);
	if (e != 0) {
		Trace("Failed to load font (facename=%s) (filename=%s)\n", facename, filename);
		return FontIDNull;
	}

	return Insert_Internal(font);
}

FontID FontStore::GetFallbackFontID() {
	FontID fid = FontIDNull;
#if XO_PLATFORM_WIN_DESKTOP
	fid = InsertByFacename("Arial");
#elif XO_PLATFORM_ANDROID
	fid = InsertByFacename("Roboto");
#elif XO_PLATFORM_LINUX_DESKTOP
	fid = InsertByFacename("Arial");
#else
	XO_TODO_STATIC;
#endif
	XO_ASSERT(fid != FontIDNull);
	return fid;
}

FontTableImmutable FontStore::GetImmutableTable() {
	std::lock_guard<std::mutex> lock(Lock);
	FontTableImmutable          t;
	t.Initialize(Fonts);
	return t;
}

void FontStore::AddFontDirectory(const char* dir) {
	std::lock_guard<std::mutex> lock(Lock);
	Directories += dir;
	IsFontTableLoaded = false;
}

const Font* FontStore::GetByFacename_Internal(const char* facename) const {
	FontID id;
	if (FacenameToFontID.get(TempString(facename), id))
		return Fonts[id];
	else
		return nullptr;
}

FontID FontStore::Insert_Internal(const Font& font) {
	Font* copy = new Font();
	*copy      = font;
	copy->ID   = (FontID) Fonts.size();
	Fonts += copy;
	FacenameToFontID.insert(copy->Facename, copy->ID);
	LoadFontConstants(*copy);
	LoadFontTweaks(*copy);
	return copy->ID;
}

#define EM_TO_256(x) ((int32_t)((x) *256 / font.FTFace->units_per_EM))

void FontStore::LoadFontConstants(Font& font) {
	uint32_t ftflags  = FT_LOAD_LINEAR_DESIGN;
	int32_t  refSize  = 100;
	FT_UInt  iFTGlyph = FT_Get_Char_Index(font.FTFace, 32);
	FT_Error e        = FT_Set_Pixel_Sizes(font.FTFace, refSize, refSize);
	XO_ASSERT(e == 0);
	e = FT_Load_Glyph(font.FTFace, iFTGlyph, ftflags);
	if (e != 0) {
		Trace("Failed to load glyph for character %d (%d)\n", 32, iFTGlyph);
		font.LinearHoriAdvance_Space_x256 = 0;
	} else {
		font.LinearHoriAdvance_Space_x256 = EM_TO_256(font.FTFace->glyph->linearHoriAdvance);
	}

	iFTGlyph = FT_Get_Char_Index(font.FTFace, 'x');
	XO_ASSERT(e == 0);
	e = FT_Load_Glyph(font.FTFace, iFTGlyph, ftflags);
	if (e != 0) {
		Trace("Failed to load glyph for character %d (%d)\n", 'x', iFTGlyph);
		font.LinearXHeight_x256 = 0;
	} else {
		font.LinearXHeight_x256 = font.FTFace->glyph->metrics.height * (256 / 64) / refSize;
	}

	font.LineHeight_x256 = EM_TO_256(font.FTFace->height);
	font.Ascender_x256   = EM_TO_256(font.FTFace->ascender);
	font.Descender_x256  = EM_TO_256(font.FTFace->descender);
}

void FontStore::LoadFontTweaks(Font& font) {
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

const char* FontStore::GetFilenameFromFacename(const char* facename) {
	if (!IsFontTableLoaded) {
		if (!LoadFontTable())
			BuildAndSaveFontTable();
	}

	String  name = facename;
	String* fn   = FacenameToFilename.getp(name);
	if (fn != nullptr)
		return fn->Z;

	name += " Regular";
	fn = FacenameToFilename.getp(name);
	if (fn != nullptr)
		return fn->Z;

	return nullptr;

	/*
	// We need to build a cache here and save it to disk.
	TempString name( facename );
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

void FontStore::BuildAndSaveFontTable() {
	Trace("Building font table on %d directories\n", (int) Directories.size());

	cheapvec<String> files;
	for (size_t i = 0; i < Directories.size(); i++) {
		auto cb = [&](const FilesystemItem& item) -> bool {
			if (IsFontFilename(item.Name))
				files += String(item.Root) + XO_DIR_SEP_STR + item.Name;
			return true;
		};

		FindFiles(Directories[i].Z, cb);
	}

	String cacheFile = Global()->CacheDir + XO_DIR_SEP_STR + "fonts";
	FILE*  manifest  = fopen(cacheFile.Z, "wb");
	if (manifest == nullptr) {
		Trace("Failed to open font cache file %s. Aborting.\n", cacheFile.Z);
		XO_DIE_MSG("Failed to open font cache file");
	}
	fprintf(manifest, "%d\n", ManifestVersion);
	fprintf(manifest, "%llu\n", (long long unsigned) ComputeFontDirHash());

	for (size_t i = 0; i < files.size(); i++) {
		FT_Face  face;
		FT_Error e = FT_New_Face(FTLibrary, files[i].Z, 0, &face);
		if (e == 0) {
			String facename     = face->family_name;
			String style        = face->style_name;
			String fullFacename = facename + " " + style;
			fprintf(manifest, "%s%c%s\n", files[i].Z, UnitSeparator, fullFacename.Z);
			FT_Done_Face(face);
		} else {
			Trace("Failed to load font (filename=%s)\n", files[i].Z);
		}
	}

	fclose(manifest);

	LoadFontTable();
}

bool FontStore::LoadFontTable() {
	XOTRACE_FONTS("LoadFontTable enter\n");

	FacenameToFilename.clear();
	FILE* manifest = fopen((Global()->CacheDir + XO_DIR_SEP_STR + "fonts").Z, "rb");
	if (manifest == nullptr)
		return false;

	XOTRACE_FONTS("LoadFontTable file loaded\n");

	int      version = 0;
	uint64_t hash    = 0;
	bool     ok      = true;
	ok               = ok && 1 == fscanf(manifest, "%d\n", &version);
	ok               = ok && 1 == fscanf(manifest, "%llu\n", (long long unsigned*) &hash);
	if (version != ManifestVersion || hash != ComputeFontDirHash()) {
		fclose(manifest);
		return false;
	}

	XOTRACE_FONTS("LoadFontTable version & hash good\n");

	// Read the rest of the file into one buffer
	cheapvec<char> remain;
	{
		char   buf[1024];
		size_t nbytes = 0;
		while ((nbytes = fread(buf, 1, sizeof(buf), manifest)) != 0)
			remain.addn(buf, nbytes);
	}

	fclose(manifest);

	// Parse the contents out line by line, using character 31 as a field separator
	const char* buf       = &remain[0];
	size_t      lineStart = 0;
	size_t      term1     = 0;
	for (size_t i = 0; true; i++) {
		if (i == remain.size() || buf[i] == '\n') {
			if (i - term1 > 1) {
				String path, facename;
				path.Set(buf + lineStart, term1 - lineStart);
				facename.Set(buf + term1 + 1, i - term1 - 1);
				FacenameToFilename.insert(facename, path);
				XOTRACE_FONTS("Font %s -> %s\n", facename.Z, path.Z);
			}
			lineStart = i + 1;
			term1     = lineStart;
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

uint64_t FontStore::ComputeFontDirHash() {
	void* hstate = XXH64_init(0);

	auto cb = [&](const FilesystemItem& item) -> bool {
		if (IsFontFilename(item.Name)) {
			XXH64_update(hstate, item.Root, (int) strlen(item.Root));
			XXH64_update(hstate, item.Name, (int) strlen(item.Name));
			XXH64_update(hstate, &item.TimeModify, sizeof(item.TimeModify));
		}
		return true;
	};

	for (size_t i = 0; i < Directories.size(); i++)
		FindFiles(Directories[i].Z, cb);

	return (uint64_t) XXH64_digest(hstate);
}

bool FontStore::IsFontFilename(const char* filename) {
	return strstr(filename, ".ttf") != nullptr ||
	       strstr(filename, ".ttc ") != nullptr;
}
}
