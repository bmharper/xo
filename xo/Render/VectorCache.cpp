#include "pch.h"
#include "VectorCache.h"
#include "../Image/Image.h"
#include "../Canvas/Canvas2D.h"
#include "../Containers/VariableTable.h"

namespace xo {

VectorCacheKey VectorCacheKey::Make(int iconID, int width, int height) {
	VectorCacheKey k;
	k.IconID = iconID;
	k.Width  = width;
	k.Height = height;
	return k;
}

VectorCache::VectorCache() {
}

VectorCache::~VectorCache() {
}

bool VectorCache::Get(int iconID, int width, int height, Elem& cached) const {
	return Get(VectorCacheKey::Make(iconID, width, height), cached);
}

bool VectorCache::Get(const VectorCacheKey& key, Elem& cached) const {
	Elem* c = Map.getp(key);
	if (!c)
		return false;
	cached = *c;
	return true;
}

void VectorCache::Set(int iconID, const Image& img) {
	Elem e = AllocAtlas(iconID, img.Width, img.Height);
	Atlases[e.Atlas].CopyInto(e.X, e.Y, img.Data, img.Stride, img.Width, img.Height);
}

VectorCache::Elem VectorCache::AllocAtlas(int iconID, int width, int height) {
	XO_ASSERT(width <= MaxSize);
	XO_ASSERT(height <= MaxSize);

	Elem e;

	for (size_t i = 0; i < Atlases.size(); i++) {
		if (Atlases[i].Alloc(width, height, e.X, e.Y)) {
			e.Atlas = (int) i;
			Map.insert(VectorCacheKey::Make(iconID, width, height), e);
			return e;
		}
	}
	Atlases.push_back(TextureAtlas());
	TextureAtlas& atlas = Atlases.back();
	uint32_t      aw    = 64;
	uint32_t      ah    = 64;
	while (aw < (uint32_t) width)
		aw *= 2;
	while (ah < (uint32_t) height)
		ah *= 2;
	atlas.Initialize(aw, ah, TexFormatRGBA8, 2);
	XO_VERIFY(atlas.Alloc(width, height, e.X, e.Y));
	e.Atlas = (int) Atlases.size() - 1;
	Map.insert(VectorCacheKey::Make(iconID, width, height), e);
	return e;
}

VectorCache::Elem VectorCache::Render(const VariableTable& vectors, VectorCacheKey key) {
	const char* svg = vectors.GetByID(key.IconID);
	if (!svg) {
		// substitute with some dummy. But we are not allowed to return false,
		// because then we enter an infinite "need another rendering pass" loop.
	}

	auto elem = AllocAtlas(key.IconID, key.Width, key.Height);
	auto tex = Atlases[elem.Atlas].Window(elem.X, elem.Y, key.Width, key.Height);
	Atlases[elem.Atlas].InvalidRect.ExpandToFit(Box(elem.X, elem.Y, key.Width, key.Height));
	Canvas2D canvas(&tex);
	canvas.RenderSVG(svg);
	return elem;
}
}
