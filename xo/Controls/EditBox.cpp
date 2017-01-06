#include "pch.h"
#include "EditBox.h"
#include "../Doc.h"
#include "../Render/RenderDoc.h"

namespace xo {
namespace controls {

void EditBox::InitializeStyles(Doc* doc) {
	doc->ClassParse("editbox", "padding: 5ep 3ep 5ep 3ep; margin: 6ep 3ep 6ep 3ep; border: 1px #bdbdbd; canfocus: true; cursor: text");
	doc->ClassParse("editbox:focus", "border: 1px #8888ee");
	doc->ClassParse("editbox.caret", "background: #0000; position: absolute; width: 1px; height: 1eh; vcenter: vcenter");
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
		s->IsBlinkVisible = !s->IsBlinkVisible;
		caret->StyleParse(s->IsBlinkVisible ? "background: #000" : "background: #0000");
	};

	auto timer = [s, edit, flip](const Event& ev) -> bool {
		if (edit->HasFocus())
			flip();
		return edit->HasFocus();
	};

	edit->OnClick([s, flip, edit, timer, caret](const Event& ev) -> bool {
		Trace("[%f %f]\n", ev.PointsRel[0].x, ev.PointsRel[0].y);
		RenderDomNode* rbox = ev.LayoutResult->IDToNode[ev.Target->GetInternalID()];
		if (rbox->Children.size() >= 1 && rbox->Children[0]->IsText()) {
			// Find the closest 'crack' in between the two nearest glyphs where we should place the caret.
			// We place our caret at the right edge of the 'crack' character
			int   crack         = 0;
			float rightOffsetPx = 0; // this is a hackish number. don't know what's best. looks like zero is best for small fonts.
			auto  rtxt          = rbox->Children[0]->ToText();
			float mindist       = FLT_MAX;
			for (size_t i = 0; i < rtxt->Text.size(); i++) {
				float dist = std::abs(PosToReal(rtxt->Text[i].X + rtxt->Text[i].Width) - ev.PointsRel[0].x);
				if (dist < mindist) {
					mindist = dist;
					crack   = (int) i;
				}
			}
			float pos;
			if (ev.PointsRel[0].x - 0 < mindist) {
				// start of text
				crack = -1;
				pos   = -rightOffsetPx;
			} else {
				// end of an existing character
				pos = PosToReal(rtxt->Text[crack].X + rtxt->Text[crack].Width);
			}
			caret->StyleParsef("left: %fpx", Round(pos + rightOffsetPx));
			if (s->TimerID)
				edit->RemoveHandler(s->TimerID);
			s->TimerID        = edit->OnTimer(timer, Global()->CaretBlinkTimeMS);
			s->IsBlinkVisible = false;
			flip();
			s->CaretPos = crack;
		}
		return true;
	});
	edit->OnGetFocus([s, flip, edit, timer](const Event& ev) -> bool {
		if (!s->TimerID)
			s->TimerID = edit->OnTimer(timer, Global()->CaretBlinkTimeMS);
		return true;
	});
	edit->OnLoseFocus([s, flip, edit](const Event& ev) -> bool {
		s->IsBlinkVisible = true;
		flip();
		if (s->TimerID)
			edit->RemoveHandler(s->TimerID);
		s->TimerID = 0;
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