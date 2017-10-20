#include "pch.h"
#include "Button.h"
#include "../Doc.h"

namespace xo {
namespace controls {

void Button::InitializeStyles(Doc* doc) {
	doc->ClassParse("xo.button", "color: #000; background: #ececec; margin: 6ep 0ep 6ep 0ep; padding: 14ep 3ep 14ep 3ep; border: 1px #bdbdbd; border-radius: 2.5ep; canfocus: true");
	doc->ClassParse("xo.button:focus", "border: 1px #8888ee");
	doc->ClassParse("xo.button:hover", "background: #ddd");
	doc->ClassParse("xo.button:capture", "border: 1px #7777ee; background: #cdcdcd");
}

DomNode* Button::NewText(DomNode* node, const char* txt) {
	return New(node, txt, nullptr, nullptr, nullptr);
}

DomNode* Button::NewSvg(DomNode* node, const char* svgIcon, const char* width, const char* height) {
	return New(node, nullptr, svgIcon, width, height);
}

DomNode* Button::New(DomNode* node, const char* txt, const char* svgIcon, const char* width, const char* height) {
	auto btn = node->AddNode(svgIcon ? xo::TagDiv : xo::TagLab);
	btn->AddClass("xo.button");

	if (txt) {
		// Add the text, even if it's empty (must be first child for DomNode.SetText to work)
		btn->AddText(txt);
	} else {
		auto svg = btn->AddNode(TagDiv);
		svg->StyleParsef("width: 100%%; height: 100%%; background: svg(%v)", svgIcon);
	}

	if (width)
		btn->StyleParsef("width: %v", width);

	if (height)
		btn->StyleParsef("height: %v", height);

	btn->OnMouseDown([btn](Event& ev) {
		btn->SetCapture();
	});

	btn->OnMouseUp([btn](Event& ev) {
		btn->ReleaseCapture();
	});
	return btn;
}

void Button::SetSvg(DomNode* node, const char* svgIcon) {
	node->NodeByIndex(0)->StyleParsef("background: svg(%v)", svgIcon);
}

void ButtonRx::RenderDiffable(std::string& content) {
	content = "<div class='xo.button'>a button</div>";
}

} // namespace controls
} // namespace xo
