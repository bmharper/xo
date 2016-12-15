#include "pch.h"
#include "DomCanvas.h"
#include "../Canvas/Canvas2D.h"
#include "../Image/Image.h"
#include "../Doc.h"

namespace xo {

DomCanvas::DomCanvas(Doc* doc, InternalID parentID) : DomNode(doc, TagCanvas, parentID) {
}

DomCanvas::~DomCanvas() {
	if (ImageName != "")
		Doc->Images.Delete(ImageName.Z);
}

void DomCanvas::CloneSlowInto(DomEl& c, uint32_t cloneFlags) const {
	DomNode::CloneSlowInto(c, cloneFlags);

	DomCanvas& cc = static_cast<DomCanvas&>(c);
	cc.ImageName  = ImageName;
}

bool DomCanvas::SetImageSizeOnly(uint32_t width, uint32_t height) {
	if ((width == 0 || height == 0) && ImageName == "")
		return true;

	Image* img = Doc->Images.Get(ImageName.Z);
	if (img == nullptr) {
		img = new Image();
		if (!img->Alloc(TexFormatRGBA8, width, height)) {
			delete img;
			return false;
		}
		//img->TexFilterMin = TexFilterNearest;
		//img->TexFilterMax = TexFilterNearest;
		img->TexFilterMin = TexFilterLinear;
		img->TexFilterMax = TexFilterLinear;
		ImageName         = Doc->Images.SetAnonymous(img);
		return true;
	} else {
		return img->Alloc(TexFormatRGBA8, width, height);
	}
}

bool DomCanvas::SetSize(uint32_t width, uint32_t height) {
	if (!SetImageSizeOnly(width, height))
		return false;
	StyleParsef("width: %upx; height: %upx", width, height);
	return true;
}

Canvas2D* DomCanvas::GetCanvas2D() {
	return new Canvas2D(Doc->Images.Get(ImageName.Z));
}

void DomCanvas::ReleaseCanvas(Canvas2D* canvas2D) {
	Image* img = canvas2D->GetImage();
	if (img != nullptr)
		img->TexInvalidRect.ExpandToFit(canvas2D->GetInvalidRect());
	delete canvas2D;
}

const char* DomCanvas::GetCanvasImageName() const {
	return ImageName.Z;
}
}
