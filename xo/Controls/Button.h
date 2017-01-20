#pragma once

namespace xo {
class Doc;
class DomNode;
namespace controls {

// A button
class XO_API Button {
public:
	static void     InitializeStyles(Doc* doc);
	static DomNode* AppendTo(DomNode* node, const char* txt = nullptr);
};
}
}