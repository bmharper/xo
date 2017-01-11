#pragma once

namespace xo {
class Doc;
class DomNode;
namespace controls {

// A modal message box
class XO_API MsgBox {
public:
	static void InitializeStyles(Doc* doc);
	static void Show(Doc* doc, const char* msg);
};
}
}
