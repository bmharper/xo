#pragma once
#include "../Defs.h"
#include "TextDefs.h"

namespace xo {

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

	void        Initialize(const cheapvec<Font*>& fonts);
	const Font* GetByFontID(FontID fontID) const;

protected:
	cheapvec<Font*> Fonts;
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
	const Font*        GetByFacename(const char* facename);
	FontID             InsertByFacename(const char* facename); // This is safe to call if the font is already loaded
	FontID             GetFallbackFontID();                    // This is a font that is always available on this platform. Panics if the font is not available.
	FontTableImmutable GetImmutableTable();

	void AddFontDirectory(const char* dir);

private:
	std::mutex                 Lock;
	cheapvec<Font*>            Fonts;
	cheapvec<String>           Directories;
	ohash::map<String, FontID> FacenameToFontID;
	ohash::map<String, String> FacenameToFilename;
	FT_Library                 FTLibrary;
	bool                       IsFontTableLoaded;

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
}
