#include "pch.h"
#include "EditBox.h"
#include "../Doc.h"
#include "../Render/RenderDoc.h"

namespace xo {
namespace controls {

static int ClampCaretPos(int p, size_t strLen) {
	return Clamp(p, -1, (int) strLen - 1);
}

void EditBox::InitializeStyles(Doc* doc) {
	doc->ClassParse("xo.editbox", "padding: 5ep 3ep 5ep 3ep; margin: 6ep 3ep 6ep 3ep; border: 1px #bdbdbd; canfocus: true; cursor: text; height: 1eh");
	doc->ClassParse("xo.editbox:focus", "border: 1px #8888ee");
	doc->ClassParse("xo.editbox.caret", "background: #0000; position: absolute; width: 1px; height: 1eh; vcenter: vcenter");
}

DomNode* EditBox::AppendTo(DomNode* node, const char* txt) {
	auto edit = node->AddNode(xo::TagLab);
	edit->AddClass("xo.editbox");

	// Add the empty text value (must be first child for DomNode.SetText to work)
	edit->AddText(txt);

	// Create the caret
	auto caret = edit->AddNode(xo::TagDiv);
	caret->AddClass("xo.editbox.caret");

	State* s     = new State();
	s->EditBoxID = edit->GetInternalID();
	s->CaretID   = caret->GetInternalID();

	auto flip = [s](Doc* doc) {
		s->IsBlinkVisible = !s->IsBlinkVisible;
		auto caret        = doc->GetNodeByInternalIDMutable(s->CaretID);
		caret->StyleParse(s->IsBlinkVisible ? "background: #000" : "background: #0000");
	};

	auto timer = [s, edit, flip](Event& ev) {
		if (edit->HasFocus())
			flip(ev.Doc);
		if (!edit->HasFocus())
			ev.CancelTimer();
	};

	auto changeText = [s, flip, edit](xo::Doc* doc, const std::string& txt) {
		if (txt != edit->GetText())
			edit->SetText(txt.c_str());

		// always show the caret after pressing any key
		s->IsBlinkVisible = false;
		flip(doc);

		// We need the DOM to call us back when it's finished rendering, because we don't yet have
		// the RenderDomText node that we need in order to place our caret.
		s->PlaceCaretOnNextRender = true;
	};

	edit->OnClick([s, flip, edit, timer](Event& ev) {
		//Trace("[%f %f]\n", ev.PointsRel[0].x, ev.PointsRel[0].y);
		RenderDomNode* rbox = ev.LayoutResult->IDToNodeTable[ev.Target->GetInternalID()];
		if (rbox->Children.size() >= 1 && rbox->Children[0]->IsText()) {
			// Find the closest 'crack' in between the two nearest glyphs where we should place the caret.
			// We place our caret at the right edge of the 'crack' character
			auto  rtxt    = rbox->Children[0]->ToText();
			float mindist = FLT_MAX;
			int   crack   = 0;
			for (size_t i = 0; i < rtxt->Text.size(); i++) {
				float dist = std::abs(PosToReal(rtxt->Text[i].X + rtxt->Text[i].Width) - ev.PointsRel[0].x);
				if (dist < mindist) {
					mindist = dist;
					crack   = (int) i;
				}
			}
			// -1 = on the left side of all our text
			if (ev.PointsRel[0].x - 0 < mindist)
				crack = -1;
			s->CaretPos = crack;
			PlaceCaretIndicator(s, ev);
			// restart the timer, so that the caret is visible for the next 500ms or so, after the user clicked
			if (s->TimerID)
				edit->RemoveHandler(s->TimerID);
			s->TimerID        = edit->OnTimer(timer, Global()->CaretBlinkTimeMS);
			s->IsBlinkVisible = false;
			flip(ev.Doc);
		}
	});
	edit->OnKeyDown([s, changeText, edit](Event& ev) {
		std::string txt         = edit->GetText();
		int         insertPos   = Clamp((int) s->CaretPos + 1, 0, (int) txt.length());
		int         newCaretPos = ClampCaretPos(s->CaretPos, txt.length());

		switch (ev.Button) {
		case (Button)((int) Button::KeyA + ('v' - 'a')):
			if (ev.IsPressed(Button::KeyCtrl)) {
				// hard-coded Ctrl+V for paste. I just need this too much right now!
				auto clipboard = ClipboardRead();
				txt.insert(insertPos, clipboard);
				newCaretPos += (int) clipboard.length();
			}
			break;
		case Button::KeyBack:
			// backspace
			if (insertPos > 0) {
				txt.erase(insertPos - 1, 1);
				//s->CaretPos--;
				newCaretPos--;
			}
			break;
		case Button::KeyLeft:
			newCaretPos--;
			break;
		case Button::KeyRight:
			newCaretPos++;
			break;
		case Button::KeyDelete: {
			int p = Clamp(s->CaretPos + 1, 0, (int) txt.length());
			if (p < (int) txt.length() && txt.length() != 0) {
				int len = utfz::seq_len(txt[p]);
				if (len == utfz::invalid) {
					txt.erase(p, 1);
				} else {
					len = Min(len, (int) txt.length() - p);
					txt.erase(p, len);
				}
			}
			break;
		}
		}
		s->CaretPos = ClampCaretPos(newCaretPos, txt.length());

		changeText(ev.Doc, txt);
	});
	edit->OnKeyChar([s, changeText, edit](Event& ev) {
		std::string txt       = edit->GetText();
		int         insertPos = Clamp((int) s->CaretPos + 1, 0, (int) txt.length());
		std::string newChar;
		utfz::encode(newChar, ev.KeyChar);
		txt.insert(insertPos, newChar);
		s->CaretPos = ClampCaretPos(s->CaretPos + 1, txt.length());
		changeText(ev.Doc, txt);
	});
	edit->OnGetFocus([s, flip, edit, timer](Event& ev) {
		if (!s->TimerID)
			s->TimerID = edit->OnTimer(timer, Global()->CaretBlinkTimeMS);
	});
	edit->OnRender([s](Event& ev) {
		if (s->PlaceCaretOnNextRender) {
			s->PlaceCaretOnNextRender = false;
			PlaceCaretIndicator(s, ev);
		}
	});
	edit->OnLoseFocus([s, flip, edit](Event& ev) {
		s->IsBlinkVisible = true;
		flip(ev.Doc);
		if (s->TimerID)
			edit->RemoveHandler(s->TimerID);
		s->TimerID = 0;
	});
	edit->OnDestroy([s](Event& ev) {
		delete s;
	});
	return edit;
}

void EditBox::PlaceCaretIndicator(State* s, Event& ev) {
	auto rbox  = ev.LayoutResult->Node(s->EditBoxID);
	auto caret = ev.Doc->GetNodeByInternalIDMutable(s->CaretID);
	if (caret) {
		float pos = 0;
		if (rbox->Children.size() >= 1 && rbox->Children[0]->IsText()) {
			auto rtxt    = rbox->Children[0]->ToText();
			int  clamped = Clamp(s->CaretPos, -1, (int) rtxt->Text.size() - 1);
			if ((size_t) clamped < (size_t) rtxt->Text.size())
				pos = PosToReal(rtxt->Text[clamped].X + rtxt->Text[clamped].Width);
			xo::Trace("clamped: %d, pos: %f\n", clamped, pos);
		}
		caret->StyleParsef("left: %fpx", Round(pos));
	}
}
}
}