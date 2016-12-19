#pragma once
namespace xo {

class Image;

// Store a set of named images.
// TODO: Improve performance of CloneMetadataFrom(). It always blows away the entire table and recreates it from scratch.
class XO_API ImageStore {
public:
	static const char* NullImageName; // You may not use this name "NULL", because it is reserved for the one-and-only null image

	ImageStore();
	~ImageStore();

	void             Set(const char* name, Image* img);
	String           SetAnonymous(Image* img); // Creates a unique name for the image, inserts the image, and returns the name
	Image*           Get(const char* name) const;
	Image*           GetOrNull(const char* name) const;
	const Image*     GetNull() const; // Get the 'null' image, which is a 2x2 checkerboard
	void             Delete(const char* name);
	cheapvec<Image*> InvalidList() const;

	void CloneMetadataFrom(const ImageStore& src);

protected:
	static const int NullImageIndex = 0;

	cheapvec<Image*>        ImageList;
	ohash::map<String, int> NameToIndex;
	cheapvec<int>           FreeIndices;
	int64_t                 NextAnon;
};
}
