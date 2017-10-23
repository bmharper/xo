#pragma once

namespace xo {
class Doc;
class DomNode;
namespace controls {

// A modal message box
class XO_API MsgBox {
public:
	DomNode*              Root = nullptr;
	std::function<void()> OnClose;

	static void InitializeStyles(Doc* doc);
	static void Show(Doc* doc, const char* msg, const char* okMsg = nullptr);

	void Create(Doc* doc, const char* msg, const char* okMsg = nullptr);
	void SetText(const char* msg);
};
} // namespace controls
} // namespace xo
