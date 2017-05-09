#include "pch.h"
#include "ImageStore.h"
#include "Image.h"

namespace xo {

ImageStore::ImageStore() {
	XO_ASSERT(0 == ImageIDNull);
	XO_ASSERT(Names.GetStr(0) == nullptr);
	Images.push_back(nullptr);
}

ImageStore::~ImageStore() {
	DeleteAll(Images);
}

ImageID ImageStore::Set(const char* name, Image* img) {
	int id = Names.GetOrCreateID(name);
	if (id == Images.size()) {
		Images.push_back(nullptr);
		Set(id, img);
	} else {
		Set(id, img);
	}
	return id;
}

void ImageStore::Set(ImageID id, Image* img) {
	// This function may only be used to modify existing images
	XO_ASSERT((size_t) id < Images.size());

	delete Images[id];
	Images[id] = img;
}

ImageID ImageStore::SetAnonymous(Image* img) {
	// Ideally, we wouldn't have to store any name for anonymous images, because we really
	// don't ever care about those names. However, the design of StringTableGC would probably
	// be more complicated if it were to support the concept of unnamed things. Most critically,
	// StringTableGC is what governs our IDs, and it has a garbage system, etc, etc, so until
	// this becomes a problem, I don't think it's worthwhile spending more energy on it.
	// The intended use case for anonymous images is Canvas objects, and I think that a few
	// bytes for this anonymous name is a small price to pay given the bigger picture of
	// the resources that are consumed by the canvas object.
	char buf[64] = "!~";
	while (true) {
		Itoa(NextAnon++, buf + 2, 36);
		if (Names.GetID(buf) == 0) {
			return Set(buf, img);
		}
	}
	XO_DIE_MSG("ImageStore failed to generate anonymous name");
	return ImageIDNull;
}

Image* ImageStore::Get(const char* name) const {
	return Images[Names.GetID(name)];
}

Image* ImageStore::Get(ImageID id) const {
	if ((size_t) id >= (size_t) Images.size())
		return nullptr;
	return Images[id];
}

void ImageStore::Delete(const char* name) {
	Delete(Names.GetID(name));
}

void ImageStore::Delete(ImageID id) {
	XO_ASSERT((size_t) id < Images.size());
	delete Images[id];
	Images[id] = nullptr;
}

cheapvec<Image*> ImageStore::InvalidList() const {
	cheapvec<Image*> invalid;
	for (size_t i = 0; i < Images.size(); i++) {
		if (Images[i] && Images[i]->InvalidRect.IsAreaPositive())
			invalid += Images[i];
	}
	return invalid;
}

void ImageStore::CloneMetadataFrom(const ImageStore& src) {
	Names.CloneFrom_Incremental(src.Names);
	while (Images.size() < src.Images.size())
		Images.push_back(nullptr);

	for (size_t i = 0; i < src.Images.size(); i++) {
		if (src.Images[i]) {
			if (!Images[i])
				Images[i] = new Image();
			Images[i]->CloneMetadataFrom(*src.Images[i]);
		} else {
			// We could do this, but why? Just unnecessary churn, for a few bytes saved.
			// delete Images[i];
			// Images[i] = nullptr;
		}
	}
}
}
