#include "pch.h"
#include "FontStore.h"

namespace xo {

static const int  ManifestVersion = 1;
static const char UnitSeparator   = 31;

FontTableImmutable::FontTableImmutable() {
}

FontTableImmutable::~FontTableImmutable() {
}

void FontTableImmutable::Initialize(const cheapvec<Font*>& fonts, const ohash::map<FontIDWeightPair, FontID>& cacheByWeight) {
	Fonts         = fonts;
	CacheByWeight = cacheByWeight;
}

const Font* FontTableImmutable::GetByFontID(FontID fontID) const {
	XO_ASSERT(fontID != FontIDNull && (size_t) fontID < Fonts.size());
	return Fonts[fontID];
}

const Font* FontTableImmutable::GetByFontIDAndWeight(FontID fontID, uint8_t weight) const {
	if (weight == 4)
		return GetByFontID(fontID);

	uint32_t key = MakeFontIDWeightPair(fontID, weight);
	fontID       = CacheByWeight.get(key);
	if (fontID == FontIDNull)
		return nullptr;
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
#elif XO_PLATFORM_OSX
	Directories += "/Library/Fonts";
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
	std::lock_guard<std::mutex> lock(Lock);
	XO_ASSERT(fontID != FontIDNull && (size_t) fontID < Fonts.size());
	return Fonts[fontID];
}

const Font* FontStore::GetByFontIDAndWeight(FontID fontID, uint8_t weight) {
	const Font* plain = GetByFontID(fontID);
	if (!plain || weight == 4)
		return plain;

	uint32_t cacheKey = MakeFontIDWeightPair(fontID, weight);
	{
		Lock.lock();
		FontID fromAccel = CacheByWeight.get(cacheKey);
		Lock.unlock();
		if (fromAccel != 0)
			return GetByFontID(fromAccel);
	}

	// Someday we'll probably want a bunch of different matching names here for the different categories,
	// such as "extralight" = "light", and "heavy" = "black"
	const char* lut[9] = {
	    " thin",
	    " light",
	    " semilight",
	    " regular",
	    " medium",
	    " semibold",
	    " bold",
	    " extrabold",
	    " black",
	};

	String face = plain->Facename;
	face.MakeLower();
	const Font* fnt   = nullptr;
	int         delta = 0;
	for (int i = 0; i < 5; i++) { // the 5 here takes us up to -2 from where we wanted to be
		int pos = weight - 1 + delta;
		if (pos < 0 || pos >= arraysize(lut))
			break;
		fnt = GetByFacename((plain->Facename + lut[pos]).CStr());
		if (fnt) {
			Trace("Resolved weighted font %s @ %d -> %s.\n", plain->Facename.CStr(), (int) weight * 100, fnt->Facename.CStr());
			Lock.lock();
			CacheByWeight.insert(cacheKey, fnt->ID);
			Lock.unlock();
			return fnt;
		}

		// produce 0,1,-1,2,-2,3,-3,4,-4
		if (delta == 0)
			delta = 1;
		else if (delta > 0)
			delta = -delta;
		else
			delta = -delta + 1;
	}

	Trace("Failed to load font %s, weight %d. Using font as-is.\n", plain->Facename.CStr(), (int) weight * 100);
	Lock.lock();
	CacheByWeight.insert(cacheKey, plain->ID);
	Lock.unlock();
	return plain;
}

const Font* FontStore::GetByFacename(const char* facename) {
	auto id = InsertByFacename(facename);
	if (id == 0)
		return nullptr;
	std::lock_guard<std::mutex> lock(Lock);
	return Fonts[id];
}

FontID FontStore::InsertByFacename(const char* facename) {
	std::lock_guard<std::mutex> lock(Lock);

	const Font* existing = GetByFacename_Internal(facename);
	if (existing)
		return existing->ID;

	const char* filename = GetFilenameFromFacename(facename);

	if (filename == nullptr) {
		Trace("Failed to load font (facename=%s) (font not found)\n", facename);
		return FontIDNull;
	}

	FT_Face  face;
	FT_Error e = FT_New_Face(FTLibrary, filename, 0, &face);
	if (e != 0) {
		Trace("Failed to load font (facename=%s) (filename=%s)\n", facename, filename);
		return FontIDNull;
	}

	return Insert_Internal(facename, face);
}

FontID FontStore::GetFallbackFontID() {
	FontID fid = FontIDNull;
#if XO_PLATFORM_WIN_DESKTOP
	fid = InsertByFacename("Arial");
#elif XO_PLATFORM_ANDROID
	fid = InsertByFacename("Roboto");
#elif XO_PLATFORM_LINUX_DESKTOP
	fid = InsertByFacename("Ubuntu");
	if (fid == FontIDNull)
		fid = InsertByFacename("Ubuntu Regular");
	if (fid == FontIDNull)
		fid = InsertByFacename("Arial");
#elif XO_PLATFORM_OSX
	fid = InsertByFacename("San Francisco");
	if (fid == FontIDNull)
		fid = InsertByFacename("Helvetica Neue");
	if (fid == FontIDNull)
		fid = InsertByFacename("Lucida Grande");
#else
	XO_TODO_STATIC;
#endif
	XO_ASSERT(fid != FontIDNull);
	return fid;
}

FontTableImmutable FontStore::GetImmutableTable() {
	std::lock_guard<std::mutex> lock(Lock);
	FontTableImmutable          t;
	t.Initialize(Fonts, CacheByWeight);
	return t;
}

void FontStore::AddFontDirectory(const char* dir) {
	std::lock_guard<std::mutex> lock(Lock);
	Directories += dir;
	IsFontTableLoaded = false;
}

const Font* FontStore::GetByFacename_Internal(const char* facename) const {
	FontID id;
	String low = facename;
	low.MakeLower();
	if (FacenameToFontID.get(low, id))
		return Fonts[id];

	return nullptr;
}

FontID FontStore::Insert_Internal(const char* facename, FT_Face face) {
	Font* font     = new Font();
	font->Facename = facename;
	font->FTFace   = face;
	font->ID       = (FontID) Fonts.size();
	Fonts += font;
	String low = facename;
	low.MakeLower();
	FacenameToFontID.insert(low, font->ID);
	LoadFontConstants(*font);
	LoadFontTweaks(*font);
	return font->ID;
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

	// UPDATE - the above is pre freetype 2.6.2. Need to re-evaluate

	// The 's' is screwed up at many sizes with Freetype >= 2.6.2, and <= 2.8 (latest now is 2.8)
	// It's not only at small sizes - I've seen the screwed up 's' at pretty large sizes.. I think around 30px
	if (font.Facename == "Ubuntu")
		font.MaxAutoHinterSize = 100;

	// if (font.Facename == "Times New Roman")
	// 	font.MaxAutoHinterSize = 30;

	//if (font.Facename == "Microsoft Sans Serif")
	//	font.MaxAutoHinterSize = 14;

	// This always looks better with the TT hinter, but 0 is now the default for MaxAutoHinterSize
	//if (font.Facename == "Segoe UI")
	//	font.MaxAutoHinterSize = 30;

	// This always looks better with the TT hinter
	//if ( font.Facename == "Audiowide" )
	//	font.MaxAutoHinterSize = 50;
}

const char* FontStore::GetFilenameFromFacename(const char* facename) {
	if (!IsFontTableLoaded) {
		if (!LoadFontTable())
			BuildAndSaveFontTable();
	}

	String name = facename;
	name.MakeLower();
	String* fn = FacenameToFilename.getp(name);
	if (fn != nullptr)
		return fn->Z;

	name += " regular";
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

		FindFiles(Directories[i].CStr(), cb);
	}

	String cacheFile = Global()->CacheDir + XO_DIR_SEP_STR + "fonts";
	FILE*  manifest  = fopen(cacheFile.CStr(), "wb");
	if (manifest == nullptr) {
		Trace("Failed to open font cache file %s. Aborting.\n", cacheFile.CStr());
		XO_DIE_MSG("Failed to open font cache file");
	}
	fprintf(manifest, "%d\n", ManifestVersion);
	fprintf(manifest, "%llu\n", (long long unsigned) ComputeFontDirHash());

	for (size_t i = 0; i < files.size(); i++) {
		FT_Face  face;
		FT_Error e = FT_New_Face(FTLibrary, files[i].CStr(), 0, &face);
		if (e == 0) {
			String facename     = face->family_name;
			String style        = face->style_name;
			String fullFacename = facename + " " + style;
			fprintf(manifest, "%s%c%s\n", files[i].CStr(), UnitSeparator, fullFacename.CStr());
			FT_Done_Face(face);
		} else {
			Trace("Failed to load font (filename=%s)\n", files[i].CStr());
		}
	}

	fclose(manifest);

	LoadFontTable();
}

bool FontStore::LoadFontTable() {
	XOTRACE_FONTS("LoadFontTable enter\n");

	FacenameToFilename.clear();
	FILE* manifest = fopen((Global()->CacheDir + XO_DIR_SEP_STR + "fonts").CStr(), "rb");
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
				XOTRACE_FONTS("Font %s -> %s\n", facename.CStr(), path.CStr());
				facename.MakeLower();
				Trace("Font %s -> %s\n", facename.CStr(), path.CStr());
				FacenameToFilename.insert(facename, path);
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
		FindFiles(Directories[i].CStr(), cb);

	return (uint64_t) XXH64_digest(hstate);
}

bool FontStore::IsFontFilename(const char* filename) {
	return strstr(filename, ".ttf") != nullptr ||
	       strstr(filename, ".ttc ") != nullptr;
}
} // namespace xo
