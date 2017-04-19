#include "pch.h"
#include "DomCanvas.h"
#include "../Canvas/Canvas2D.h"
#include "../Image/Image.h"
#include "../Doc.h"

namespace xo {

DomCanvas::DomCanvas(xo::Doc* doc, xo::InternalID parentID) : DomNode(doc, TagCanvas, parentID) {
}

DomCanvas::~DomCanvas() {
	if (ImageID != ImageIDNull)
		Doc->Images.Delete(ImageID);
}

void DomCanvas::CloneSlowInto(DomEl& c, uint32_t cloneFlags) const {
	DomNode::CloneSlowInto(c, cloneFlags);

	DomCanvas& cc = static_cast<DomCanvas&>(c);
	cc.ImageID    = ImageID;
}

bool DomCanvas::SetImageSizeOnly(uint32_t width, uint32_t height) {
	if ((width == 0 || height == 0) && ImageID == ImageIDNull)
		return true;

	Image* img = Doc->Images.Get(ImageID);
	if (img == nullptr) {
		img = new Image();
		if (!img->Alloc(TexFormatRGBA8, width, height)) {
			delete img;
			return false;
		}
		//img->TexFilterMin = TexFilterNearest;
		//img->TexFilterMax = TexFilterNearest;
		img->FilterMin = TexFilterLinear;
		img->FilterMax = TexFilterLinear;
		ImageID        = Doc->Images.SetAnonymous(img);
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

void DomCanvas::Fill(xo::Color color) {
	auto c = GetCanvas2D();
	c->Fill(color);
	ReleaseCanvas(c);
}

Canvas2D* DomCanvas::GetCanvas2D() {
	return new Canvas2D(Doc->Images.Get(ImageID));
}

void DomCanvas::ReleaseAndInvalidate(Canvas2D* canvas2D) {
	canvas2D->Invalidate();
	ReleaseCanvas(canvas2D);
}

void DomCanvas::ReleaseCanvas(Canvas2D* canvas2D) {
	auto img = canvas2D->GetImage();
	if (img != nullptr)
		img->InvalidRect.ExpandToFit(canvas2D->GetInvalidRect());
	delete canvas2D;
	IncVersion();
}

} // namespace xo
