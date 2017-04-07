#pragma once

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
};
}
}