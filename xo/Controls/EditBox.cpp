#include "pch.h"
#include "EditBox.h"
#include "../Doc.h"

namespace xo {
namespace controls {

void EditBox::InitializeStyles(Doc* doc) {
	doc->ClassParse("editbox", "padding: 5ep 3ep 5ep 3ep; margin: 6ep 3ep 6ep 3ep; border: 1px #bdbdbd; canfocus: true; cursor: text");
	doc->ClassParse("editbox:focus", "border: 1px #8888ee");
	doc->ClassParse("editbox.caret", "background: #0000; position: absolute; width: 1px; height: 1.2em; vcenter: vcenter");
}

DomNode* EditBox::AppendTo(DomNode* node) {
	auto edit = node->AddNode(xo::TagLab);
	edit->AddClass("editbox");

	// Add the empty text value
	edit->AddText();

	// Create the caret
	auto caret = edit->AddNode(xo::TagDiv);
	caret->AddClass("editbox.caret");

	State* s = new State();

	auto flip = [s, caret]() {
		s->IsBlinked = !s->IsBlinked;
		caret->StyleParse(s->IsBlinked ? "background: #000" : "background: #0000");
		TimeTrace("Caret flip %s\n", s->IsBlinked ? "on" : "off");
	};

	auto timer = [s, edit, flip](const Event& ev) -> bool {
		if (edit->HasFocus())
			flip();
		return edit->HasFocus();
	};

	edit->OnGetFocus([s, flip, edit, timer](const Event& ev) -> bool {
		uint32_t caretTickIntervalMS = 500;
		flip();
		edit->OnTimer(timer, caretTickIntervalMS);
		return true;
	});
	edit->OnLoseFocus([s, flip](const Event& ev) -> bool {
		s->IsBlinked = true;
		flip();
		return true;
	});
	edit->OnDestroy([s](const Event& ev) -> bool {
		delete s;
		return true;
	});
	return edit;
}
}
}