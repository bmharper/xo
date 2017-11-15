#pragma once

#include "../Reactive/Control.h"

namespace xo {
class Doc;
class DomNode;
namespace controls {

// A button
class XO_API Button {
public:
	static void     InitializeStyles(Doc* doc);
	static DomNode* NewText(DomNode* node, const char* txt);
	static DomNode* NewSvg(DomNode* node, const char* svgIcon, const char* width, const char* height);
	static DomNode* New(DomNode* node, const char* txt, const char* svgIcon, const char* width, const char* height); // txt OR svg

	static void SetSvg(DomNode* node, const char* svgIcon);
};

// A button
class XO_API ButtonRx : public xo::rx::Control {
public:
	static xo::rx::Control* Create() { return new ButtonRx(); }

	virtual void Render(std::string& dom);
};

} // namespace controls
} // namespace xo