#pragma once
#include "DomNode.h"

namespace xo {

// DOM node that owns an Canvas2D object
// The Canvas is destroyed with the DOM node. There are other ways of achieving similar behaviour,
// ie using a background image style. The primary thing that the canvas DOM element brings
// is binding the lifetime of the canvas image to the DOM node.
class XO_API DomCanvas : public DomNode {
public:
	DomCanvas(xo::Doc* doc, xo::InternalID parentID);
	virtual ~DomCanvas();

	virtual void CloneSlowInto(DomEl& c, uint32_t cloneFlags) const override;

	bool        SetImageSizeOnly(uint32_t width, uint32_t height); // Sets the size of the image only (DOM styles are unaffected). Returns false if memory allocation fails.
	bool        SetSize(uint32_t width, uint32_t height);          // Convenience function to set width/height styles, as well as resize the canvas object. Returns false if memory allocation fails
	Canvas2D*   GetCanvas2D();
	void        ReleaseCanvas(Canvas2D* canvas2D);
	const char* GetCanvasImageName() const;

protected:
	//Canvas2D*		Canvas2D = nullptr;
	String ImageName;
};
}
