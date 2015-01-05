#pragma once
#include "xoDomNode.h"

// DOM node that owns an xoCanvas2D object
// The Canvas is destroyed with the DOM node. There are other ways of achieving similar behaviour,
// ie using a background image style. The primary thing that the canvas DOM element brings
// is binding the lifetime of the canvas image to the DOM node.
class XOAPI xoDomCanvas : public xoDomNode
{
public:
	xoDomCanvas(xoDoc* doc, xoInternalID parentID);
	virtual			~xoDomCanvas();

	virtual void	CloneSlowInto(xoDomEl& c, uint cloneFlags) const override;

	bool			SetImageSizeOnly(uint width, uint height);	// Sets the size of the image only (DOM styles are unaffected). Returns false if memory allocation fails.
	bool			SetSize(uint width, uint height);				// Convenience function to set width/height styles, as well as resize the canvas object. Returns false if memory allocation fails
	xoCanvas2D*		GetCanvas2D();
	void			ReleaseCanvas(xoCanvas2D* canvas2D);
	const char*		GetCanvasImageName() const;

protected:
	//xoCanvas2D*		Canvas2D = nullptr;
	xoString	ImageName;
};
