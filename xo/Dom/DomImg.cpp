#include "pch.h"
#include "DomImg.h"
#include "../Image/Image.h"
#include "../Doc.h"

namespace xo {

DomImg::DomImg(xo::Doc* doc, xo::InternalID parentID) : DomNode(doc, TagImg, parentID) {
}

DomImg::~DomImg() {
	if (ImageID != ImageIDNull)
		Doc->Images.Delete(ImageID);
}

void DomImg::CloneSlowInto(DomEl& c, uint32_t cloneFlags) const {
	DomNode::CloneSlowInto(c, cloneFlags);

	DomImg& cc = static_cast<DomImg&>(c);
	cc.ImageID = ImageID;
}
}
