#pragma once

#include "../Defs.h"
#include "../Containers/StringTableGC.h"

namespace xo {

class Image;

/* Set of named images.

You can think of this as the texture table for the document.

The Canvas object uses anonymous images in here to store its actual texels.

The images in here are uploaded to GPU textures by DocGroup::UploadImagesToGPU,
during an initial rendering phase.

IDEA: Change the anonymous image concept, so that there is a separate pool for
anonymous images. Use a tag bit inside ImageID to flag an ID as being anonymous.
*/
class XO_API ImageStore {
public:
	ImageStore();
	~ImageStore();

	ImageID          Set(const char* name, Image* img); // Create or modify an image
	void             Set(ImageID id, Image* img);       // Replace an existing image. Normally you'll just update the image by manipulating it and setting its dirty rect.
	ImageID          SetAnonymous(Image* img);          // Insert an unnamed image, and returns the ID
	Image*           Get(const char* name) const;
	Image*           Get(ImageID id) const;
	void             Delete(const char* name);
	void             Delete(ImageID id);
	cheapvec<Image*> InvalidList() const; // The list of images that have been modified since the last GPU upload

	// This only clones the metadata, because the actual texels are never cloned in system memory,
	// but they are cloned into GPU memory by DocGroup::UploadImagesToGPU(). It might be good
	// someday to mark a texture as "write-only", which would be an instruction to xo that
	// it can discard the system memory for a texture once the texture has been uploaded to the GPU.
	// Of course, you can lose your GPU surface, so you might need to accompany such a feature with
	// a callback that will allow the client application to reload a lost image.
	void CloneMetadataFrom(const ImageStore& src);

protected:
	cheapvec<Image*> Images;
	StringTableGC    Names;
	uint64_t         NextAnon = 1;
};
}
