#pragma once

#include "Defs.h"
#include "TextureAtlas.h"

namespace xo {

class VariableTable;

struct VectorCacheKey {
	int                   IconID;
	int                   Width;
	int                   Height;
	static VectorCacheKey Make(int iconID, int width, int height);
	ohash::hashkey_t      GetHashCode() const { return IconID ^ (Width << 14) ^ (Height << 23); }
	bool                  operator==(const VectorCacheKey& s) const { return IconID == s.IconID && Width == s.Width && Height == s.Height; }
};

// Cache of rasterized vector icons
class XO_API VectorCache {
public:
	// Element cached inside texture atlas
	struct Elem {
		int      Atlas;
		uint16_t X;
		uint16_t Y;
	};

	// Set will panic if you try to insert an item larger than this.
	// An RGBA 4096x4096 texture is 64MB.
	static const int MaxSize = 4096;

	VectorCache();
	~VectorCache();

	bool Get(int iconID, int width, int height, Elem& cached) const; // Thread safe
	bool Get(const VectorCacheKey& key, Elem& cached) const;         // Thread safe
	void Set(int iconID, const Image& img);                          // Not thread safe
	Elem AllocAtlas(int iconID, int width, int height);              // Not thread safe
	Elem Render(const VariableTable& vectors, VectorCacheKey key);   // Not thread safe

	TextureAtlas* GetAtlas(int atlas) { return &Atlases[atlas]; }

private:
	ohash::map<VectorCacheKey, Elem> Map;
	cheapvec<TextureAtlas>           Atlases;
};
}

namespace ohash {
inline ohash::hashkey_t gethashcode(const xo::VectorCacheKey& k) { return (hashkey_t) k.GetHashCode(); }
}