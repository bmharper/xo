#include "pch.h"
#include "ImageStore.h"
#include "Image.h"

namespace xo {

const char* ImageStore::NullImageName = "NULL";

ImageStore::ImageStore() {
	Image*   nimg        = new Image();
	uint32_t ndata[2][2] = {
	    {0xffffffff, 0xff000000},
	    {0xff000000, 0xffffffff},
	};
	nimg->Set(TexFormatRGBA8, 2, 2, ndata);
	XO_ASSERT(NullImageIndex == ImageList.size());
	Set(NullImageName, nimg);
	NextAnon = 1;
}

ImageStore::~ImageStore() {
	DeleteAll(ImageList);
	NameToIndex.clear();
}

void ImageStore::Set(const char* name, Image* img) {
	TempString sname(name);
	int        index  = -1;
	bool       exists = NameToIndex.get(sname, index);
	if (exists) {
		delete ImageList[index];
		ImageList[index] = img;
	} else {
		if (FreeIndices.size() != 0) {
			index            = FreeIndices.rpop();
			ImageList[index] = img;
		} else {
			index = (int) ImageList.size();
			ImageList += img;
		}

		NameToIndex.insert(sname, index);
	}
}

String ImageStore::SetAnonymous(Image* img) {
	char buf[64] = "!~";
	while (true) {
		Itoa(NextAnon++, buf + 2, 36);
		if (!NameToIndex.contains(TempString(buf))) {
			Set(buf, img);
			return buf;
		}
	}
	XO_DIE_MSG("ImageStore failed to generate anonymous name");
	return "";
}

Image* ImageStore::Get(const char* name) const {
	int index = -1;
	if (NameToIndex.get(TempString(name), index))
		return ImageList[index];
	else
		return NULL;
}

Image* ImageStore::GetOrNull(const char* name) const {
	Image* img = Get(name);
	if (img)
		return img;
	return ImageList[NullImageIndex];
}

void ImageStore::Delete(const char* name) {
	int index = -1;
	if (NameToIndex.get(TempString(name), index)) {
		NameToIndex.erase(TempString(name));
		delete ImageList[index];
		ImageList[index] = nullptr;
		FreeIndices += index;
	}
}

cheapvec<Image*> ImageStore::InvalidList() const {
	cheapvec<Image*> invalid;
	for (size_t i = 0; i < ImageList.size(); i++) {
		if (ImageList[i] != nullptr && ImageList[i]->TexInvalidRect.IsAreaPositive())
			invalid += ImageList[i];
	}
	return invalid;
}

const Image* ImageStore::GetNull() const {
	return ImageList[NullImageIndex];
}

void ImageStore::CloneMetadataFrom(const ImageStore& src) {
	// TODO: Stop needless thrashing here, by blowing away the entire image store and recreating it.
	// A very simple optimization would be to simply detect if the two ImageStores are parallel. If so,
	// one can avoid recreating all of them.

	DeleteAll(ImageList);
	NameToIndex.clear();
	FreeIndices.clear();

	auto cloneImage = [](const Image* img) -> Image* {
		// we don't want the renderer to try and upload this empty shell of a texture, so we mark it "valid"
		Image* clone = img->CloneMetadata();
		clone->TexClearInvalidRect();
		return clone;
	};

	Set(NullImageName, cloneImage(src.Get(NullImageName)));

	for (auto it = src.NameToIndex.begin(); it != src.NameToIndex.end(); it++) {
		if (it->second != NullImageIndex)
			Set(it->first.Z, cloneImage(src.ImageList[it->second]));
	}
}
}
