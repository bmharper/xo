#include "pch.h"
#include "EditBox.h"
#include "../Doc.h"

namespace xo {
namespace controls {

void EditBox::InitializeStyles(Doc* doc) {
	doc->ClassParse("editbox", "padding: 5ep 3ep 5ep 3ep; margin: 6ep 3ep 6ep 3ep; border: 1px #bdbdbd; canfocus: true; cursor: text");
	//doc->ClassParse("editbox", "padding: 0; margin: 0; border: 1px #bdbdbd; canfocus: true; cursor: text");
	doc->ClassParse("editbox:focus", "border: 1px #8888ee");
	doc->ClassParse("editbox.caret", "background: #0000; position: absolute; width: 1px; height: 1.2em; vcenter: vcenter");
}

DomNode* EditBox::AppendTo(DomNode* node) {
	auto edit = node->AddNode(xo::TagLab);
	edit->AddClass("editbox");

	// Add the empty text value (must be first child for DomNode.SetText to work)
	edit->AddText();

	// Create the caret
	auto caret = edit->AddNode(xo::TagDiv);
	caret->AddClass("editbox.caret");

	State* s = new State();

	auto flip = [s, caret]() {
		s->IsBlinked = !s->IsBlinked;
		caret->StyleParse(s->IsBlinked ? "background: #000" : "background: #0000");
	};

	auto timer = [s, edit, flip](const Event& ev) -> bool {
		if (edit->HasFocus())
			flip();
		return edit->HasFocus();
	};

	edit->OnClick([s, flip, edit, timer](const Event& ev) -> bool {
		if (ev.TargetText && ev.TargetChar != -1) {
			s->CaretPos = ev.TargetChar;
		}
		return true;
	});
	edit->OnGetFocus([s, flip, edit, timer](const Event& ev) -> bool {
		flip();
		edit->OnTimer(timer, Global()->CaretBlinkTimeMS);
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