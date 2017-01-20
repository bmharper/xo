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

DomNode* Button::AppendTo(DomNode* node, const char* txt) {
	auto btn = node->AddNode(xo::TagLab);
	btn->AddClass("xo.button");

	// Add the empty text value (must be first child for DomNode.SetText to work)
	btn->AddText(txt);

	btn->OnMouseDown([btn](Event& ev) {
		btn->SetCapture();
	});

	btn->OnMouseUp([btn](Event& ev) {
		btn->ReleaseCapture();
	});
	return btn;
}
}
}
