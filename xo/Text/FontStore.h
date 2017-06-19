#pragma once
#include "../Defs.h"
#include "TextDefs.h"

namespace xo {

typedef uint32_t FontIDWeightPair;

// weight is 1..9
inline FontIDWeightPair MakeFontIDWeightPair(FontID fontID, uint8_t weight) {
	return ((uint32_t) fontID << 10) | (uint32_t) weight;
}

inline void SplitFontIDWeightPair(FontIDWeightPair pair, FontID& fontID, uint8_t& weight) {
	fontID = pair >> 10;
	weight = pair & 15;
}

// This is used during layout to get an immutable set of fonts that we can access
// without having to take any locks. Once a Font object has been created, it is
// never mutated. The Freetype internals are most definitely mutated as we generate
// more glyphs, but the info directly stored inside Font is immutable.
// Of particular importance is LinearHoriAdvance_Space_x256, which is used a lot
// during layout.
class XO_API FontTableImmutable {
public:
	FontTableImmutable();
	~FontTableImmutable();

	void        Initialize(const cheapvec<Font*>& fonts, const ohash::map<FontIDWeightPair, FontID>& cacheByWeight);
	const Font* GetByFontID(FontID fontID) const;                          // Panics if FontID is valid
	const Font* GetByFontIDAndWeight(FontID fontID, uint8_t weight) const; // Returns null if FontID+weight combo does not exist

protected:
	cheapvec<Font*>                      Fonts;
	ohash::map<FontIDWeightPair, FontID> CacheByWeight; // See FontStore's WeightCache for explanation
};

/* This stores only font metadata such as filename.

If you're looking for glyphs, they are stored inside GlyphCache.

All public members are thread safe.

Members suffixed by "_Internal" assume that the appropriate locks have been acquired.

Once a Font* object has been created, it will never be destroyed.
Also, since a Font object is immutable once created, we can return
a FontStoreTable object and know that the objects inside it are
safe to access from many threads, while we still go ahead on the main
thread and create more fonts.

Although the public API of this class is thread safe, the Font* objects returned are
most definitely not thread safe. Freetype stores a lot of glyph rendering state inside
the FT_Face object, so only one thread can use a Freetype face at a time.

TODO: Change the font cache file so that the filename includes the hash. At present,
if multiple xo applications run on the same machine, with different sets of font
directories, then they will thrash the font cache file.

*/
class XO_API FontStore {
public:
	FontStore();
	~FontStore();

	void               Clear();
	void               InitializeFreetype();
	void               ShutdownFreetype();
	const Font*        GetByFontID(FontID fontID);
	const Font*        GetByFontIDAndWeight(FontID fontID, uint8_t weight); // weight is from 1..9
	const Font*        GetByFacename(const char* facename);
	FontID             InsertByFacename(const char* facename); // This is safe to call if the font is already loaded
	FontID             GetFallbackFontID();                    // This is a font that is always available on this platform. Panics if the font is not available.
	FontTableImmutable GetImmutableTable();

	void AddFontDirectory(const char* dir);

private:
	std::mutex                   Lock;
	cheapvec<Font*>              Fonts;
	cheapvec<String>             Directories;
	ohash::map<String, FontID>   FacenameToFontID;   // Facename is lowercase
	ohash::map<String, String>   FacenameToFilename; // Facename is lowercase
	ohash::map<uint32_t, FontID> CacheByWeight;      // accelerate FontID + Weight lookups, so we don't need to go via the Facename for ("Segoe UI", 700) -> "SegoeUI Bold"
	FT_Library                   FTLibrary;
	bool                         IsFontTableLoaded;

	const Font* GetByFacename_Internal(const char* facename) const;
	FontID      Insert_Internal(const char* facename, FT_Face face);
	void        LoadFontConstants(Font& font);
	void        LoadFontTweaks(Font& font);
	const char* GetFilenameFromFacename(const char* facename);
	void        BuildAndSaveFontTable();
	bool        LoadFontTable();
	uint64_t    ComputeFontDirHash();

	static bool IsFontFilename(const char* filename);
};
} // namespace xo
